////////////////////////////////////////////////////////////////////////////////
// loot.c
////////////////////////////////////////////////////////////////////////////////

#include "loot.h"

#include <entities/entity.h>

#include <comm.h>
#include <config.h>
#include <stringbuffer.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

extern bool test_output_enabled;

LootDB* global_loot_db = NULL;

// Alloc helpers ///////////////////////////////////////////////////////////////

static void* xrealloc(void* p, size_t n) 
{
    void* q = realloc(p, n);
    if (!q && n != 0) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    return q;
}

static void loot_group_push_entry(LootGroup* group, LootEntry entry) 
{
    if (group->entry_count >= group->entry_capacity) {
        group->entry_capacity = group->entry_capacity == 0 ? 4 : 
            group->entry_capacity * 2;
        group->entries = (LootEntry*)xrealloc(group->entries, 
            sizeof(LootEntry) * group->entry_capacity);
    }
    group->entries[group->entry_count++] = entry;
}

static void loot_table_push_op(LootTable* table, LootOp op) 
{
    if (table->op_count >= table->op_capacity) {
        table->op_capacity = table->op_capacity == 0 ? 4 : 
            table->op_capacity * 2;
        table->ops = (LootOp*)xrealloc(table->ops, sizeof(LootOp) * 
            table->op_capacity);
    }
    table->ops[table->op_count++] = op;
}

// Lookup helpers //////////////////////////////////////////////////////////////

LootGroup* loot_db_find_group(LootDB* db, const char* name) 
{
    for (int i = 0; i < db->group_count; i++) {
        if (strcmp(db->groups[i].name, name) == 0) {
            return &db->groups[i];
        }
    }
    return NULL;
}

LootTable* loot_db_find_table(LootDB* db, const char* name) 
{
    for (int i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, name) == 0) {
            return &db->tables[i];
        }
    }
    return NULL;
}

static LootGroup* loot_db_add_group(LootDB* db, const char* name, int rolls, Entity* owner) 
{
    // Check for existing
    LootGroup* group = loot_db_find_group(db, name);
    if (group) {
        return group;
    }

    // Expand db if needed
    if (db->group_count >= db->group_capacity) {
        db->group_capacity = db->group_capacity == 0 ? 4 : 
            db->group_capacity * 2;
        db->groups = (LootGroup*)xrealloc(db->groups, sizeof(LootGroup) * 
            db->group_capacity);
    }

    // Add new group
    group = &db->groups[db->group_count++];
    group->owner = owner;
    group->name = str_dup(name);
    group->rolls = rolls;
    group->entries = NULL;
    group->entry_count = 0;
    group->entry_capacity = 0;
    return group;
}

static LootTable* loot_db_add_table(LootDB* db, const char* name, 
    const char* parent_name, Entity* owner) 
{
    // Check for existing
    LootTable* table = loot_db_find_table(db, name);
    if (table) {
        return table;
    }

    // Expand db if needed
    if (db->table_count >= db->table_capacity) {
        db->table_capacity = db->table_capacity == 0 ? 4 : 
            db->table_capacity * 2;
        db->tables = (LootTable*)xrealloc(db->tables, sizeof(LootTable) * 
            db->table_capacity);
    }

    // Add new table
    table = &db->tables[db->table_count++];
    table->owner = owner;
    table->name = str_dup(name);
    table->parent_name = parent_name ? str_dup(parent_name) : NULL;
    table->ops = NULL;
    table->op_count = 0;
    table->op_capacity = 0;
    table->resolved_ops = NULL;
    table->resolved_op_count = 0;
    table->_visit = 0;
    return table;
}

// Tiny Tokenizer //////////////////////////////////////////////////////////////

typedef struct {
    const char* buf;
    size_t len;
    size_t pos;
    int line;
} Tok;

static char* scarf_loot_section(FILE* fp, StringBuffer* sb) {
    // Read all until #ENDLOOT or EOF
    char line[MIL];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "#ENDLOOT", 8) == 0) {
            break;
        }
        sb_append(sb, line);
    }
    return sb->data;
}

static void tok_init(Tok* t, const char* buf, size_t len) 
{
    t->buf = buf;
    t->len = len;
    t->pos = 0;
    t->line = 1;
}

static void tok_skip_ws(Tok* t)
{
    while (t->pos < t->len && isspace((unsigned char)t->buf[t->pos])) {
        if (t->buf[t->pos] == '\n')
            t->line++;
        t->pos++;
    }
}

static bool tok_next(Tok* t, char out[MIL])
{
    tok_skip_ws(t);
    if (t->pos >= t->len)
        return false;

    size_t start = t->pos;
    while (t->pos < t->len && !isspace((unsigned char)t->buf[t->pos])) {
        t->pos++;
    }
    size_t tok_len = t->pos - start;
    if (tok_len >= MIL)
        tok_len = MIL - 1;
    memcpy(out, t->buf + start, tok_len);
    out[tok_len] = 0;
    return true;
}

static int tok_int(Tok* t, const char* what)
{
    char buf[MIL];
    if (!tok_next(t, buf)) {
        bugf("loot: expected integer for %s at line %d\n", what, t->line);
        return 0;
    }
    char* end = NULL;
    long val = strtol(buf, &end, 10);
    if (end == buf || *end != 0) {
        bugf("loot: expected integer for %s at line %d\n", what, t->line);
        return 0;
    }
    return (int)val;
}

static void tok_expect(Tok* t, const char* expected)
{
    char buf[MIL];
    if (!tok_next(t, buf) || strcmp(buf, expected) != 0) {
        bugf("loot: expected '%s' at line %d\n", expected, t->line);
    }
}

// Resolution helpers (with inheritance) ///////////////////////////////////////

static void append_loop_ops(LootOp** dest_ops, size_t* dest_count, 
    size_t* dest_capacity, LootOp* src_ops, size_t src_count) 
{
    if (src_count <= 0) {
        return;
    }

    if (*dest_count + src_count > *dest_capacity) {
        size_t new_cap = *dest_capacity ? *dest_capacity : 4;
        while (new_cap < *dest_count + src_count) 
            new_cap *= 2;
        *dest_capacity = new_cap;
        *dest_ops = (LootOp*)xrealloc(*dest_ops, sizeof(LootOp) * 
            (*dest_capacity));
    }

    memcpy(*dest_ops + *dest_count, src_ops, sizeof(LootOp) * src_count);
    *dest_count += src_count;
}

static void resolve_loot_table_dfs(LootDB* db, LootTable* table) 
{
    if (table->_visit == 1) {
        if (!test_output_enabled)
            bugf("Cycle detected in loot table inheritance for table '%s'\n", 
                table->name);
        return;
    }
    if (table->_visit == 2) {
        return; // Already resolved
    }

    table->_visit = 1; // Mark as visiting

    LootOp* tmp_ops = NULL;
    size_t tmp_count = 0;
    size_t tmp_capacity = 0;

    // Resolve parent first
    if (!IS_NULLSTR(table->parent_name)) {
        LootTable* parent = loot_db_find_table(db, table->parent_name);
        if (!parent) {
            bugf("Warning: loot table '%s' has unknown parent "
                "'%s'\n", table->name, table->parent_name);
            return;
        }

        resolve_loot_table_dfs(db, parent);
        append_loop_ops(&tmp_ops, &tmp_count, &tmp_capacity, 
            parent->resolved_ops, parent->resolved_op_count);
 
    }

    // Append own ops
    append_loop_ops(&tmp_ops, &tmp_count, &tmp_capacity, 
        table->ops, table->op_count);

    // Commit resolved ops
    free(table->resolved_ops);
    table->resolved_ops = tmp_ops;
    table->resolved_op_count = tmp_count;

    table->_visit = 2; // Mark as visited
}

void resolve_all_loot_tables(LootDB* db) 
{
    for (int i = 0; i < db->table_count; i++)
        db->tables[i]._visit = 0;
    for (int i = 0; i < db->table_count; i++)
        resolve_loot_table_dfs(db, &db->tables[i]);
}

// Parsing helpers /////////////////////////////////////////////////////////////

static void parse_group(LootDB* db, Tok* tok, Entity* owner)
{
    char s[MIL];
    char name[MIL];
    if (!tok_next(tok, name)) {
        bugf("loot: expected group name at line %d\n", tok->line);
        return;
    }

    int rolls = tok_int(tok, "group rolls");

    LootGroup* group = loot_db_add_group(db, name, rolls, owner);
    if (!group) {
        bugf("loot: failed to add group '%s' at line %d\n", name, tok->line);
        return;
    }

    // Parse entries
    while (true) {
        size_t save_pos = tok->pos;
        int save_line = tok->line;
        if (!tok_next(tok, s)) {
            return;
        }

        if (strcmp(s, "item") == 0) {
            int vnum = tok_int(tok, "vnum");
            int mn = tok_int(tok, "min");
            int mx = tok_int(tok, "max");
            tok_expect(tok, "weight");
            int wt = tok_int(tok, "weight");

            LootEntry entry = { 
                .type = LOOT_ITEM,
                .item_vnum = vnum,
                .min_qty = mn,
                .max_qty = mx,
                .weight = wt 
            };
            loot_group_push_entry(group, entry);
        }
        else if (strcmp(s, "cp") == 0) {
            int mn = tok_int(tok, "cp min");
            int mx = tok_int(tok, "cp max");
            tok_expect(tok, "weight");
            int wt = tok_int(tok, "weight");

            LootEntry entry = { 
                .type = LOOT_CP,
                .item_vnum = 0,
                .min_qty = mn,
                .max_qty = mx,
                .weight = wt 
            };
            loot_group_push_entry(group, entry);
        }
        else {
            // Not an entry, rewind and return
            tok->pos = save_pos;
            tok->line = save_line;
            return;
        }
    }
}

static void parse_table(LootDB* db, Tok* tok, Entity* owner)
{
    char s[MIL];
    char name[MIL];
    if (!tok_next(tok, name)) {
        bugf("loot: expected table name at line %d\n", tok->line);
        return;
    }

    // Optional parent
    char maybe_parent[MIL];
    size_t save_pos = tok->pos;
    int save_line = tok->line;

    char parent[MIL];
    bool has_parent = false;

    if (tok_next(tok, maybe_parent) && strcmp(maybe_parent, "parent") == 0) {
        if (!tok_next(tok, parent)) {
            bugf("loot: expected parent table name at line %d\n", tok->line);
            return;
        }
        has_parent = true;
    }
    else {
        // No parent, rewind
        tok->pos = save_pos;
        tok->line = save_line;
    }

    LootTable* table = loot_db_add_table(db, name, has_parent ? parent : NULL, owner);
    if (!table) {
        bugf("loot: failed to add table '%s' at line %d\n", name, tok->line);
        return;
    }

    // Parse ops
    while (true) {
        size_t save_pos = tok->pos;
        int save_line = tok->line;

        if (!tok_next(tok, s)) {
            // EOF
            return;
        }

        LootOp op;
        memset(&op, 0, sizeof(LootOp));

        if (strcmp(s, "use_group") == 0) {
            op.type = LOOT_OP_USE_GROUP;
            if (!tok_next(tok, s)) {
                bugf("loot: expected group name for use_group at line %d\n", 
                    tok->line);
                return;
            }
            op.group_name = str_dup(s);
            op.a = tok_int(tok, "use_group chance_pct");
        }
        else if (strcmp(s, "add_item") == 0) {
            op.type = LOOT_OP_ADD_ITEM;
            op.a = tok_int(tok, "vnum");
            op.b = tok_int(tok, "chance_pct");
            op.c = tok_int(tok, "min");
            op.d = tok_int(tok, "max");
        }
        else if (strcmp(s, "add_cp") == 0) {
            op.type = LOOT_OP_ADD_CP;
            op.a = tok_int(tok, "chance_pct");
            op.c = tok_int(tok, "min");
            op.d = tok_int(tok, "max");
        }
        else if (strcmp(s, "mul_cp") == 0) {
            op.type = LOOT_OP_MUL_CP;
            op.a = tok_int(tok, "percent");
        }
        else if (strcmp(s, "mul_all_chances") == 0) {
            op.type = LOOT_OP_MUL_ALL_CHANCES;
            op.a = tok_int(tok, "percent");
        }
        else if (strcmp(s, "remove_item") == 0) {
            op.type = LOOT_OP_REMOVE_ITEM;
            op.a = tok_int(tok, "vnum");
        }
        else if (strcmp(s, "remove_group") == 0) {
            op.type = LOOT_OP_REMOVE_GROUP;
            if (!tok_next(tok, s)) {
                bugf("loot: expected group name for remove_group at line %d\n", 
                    tok->line);
                return;
            }
            op.group_name = str_dup(s);
        }
        else {
            // Not an op, rewind and return
            tok->pos = save_pos;
            tok->line = save_line;
            return;
        }
        loot_table_push_op(table, op);
    }
}

void parse_loot_section(LootDB* db, StringBuffer* sb, Entity* owner)
{
    size_t len = sb->length;
    char* buf = sb->data;
    Tok tok;
    tok_init(&tok, buf, len);
    char s[MIL];

    while (tok_next(&tok, s)) {
        if (strcmp(s, "group") == 0) {
            parse_group(db, &tok, owner);
        }
        else if (strcmp(s, "table") == 0) {
            parse_table(db, &tok, owner);
        }
        else if (strcmp(s, "#ENDLOOT") == 0) {
            // End of loot section
            return;
        }   
        else {
            bugf("loot: unexpected token '%s' at line %d\n", s, tok.line);
        }
    }
}

void load_global_loot_db()
{
    char loot_file_path[MIL];
    sprintf(loot_file_path, "%s%s", cfg_get_data_dir(), cfg_get_loot_file());

    FILE* fp = fopen(loot_file_path, "r");
    if (!fp) {
        fprintf(stderr, "Could not open loot file: %s\n", loot_file_path);
        exit(1);
    }

    StringBuffer* sb = sb_new();
    scarf_loot_section(fp, sb);
    fclose(fp);

    LootDB* db = (LootDB*)xrealloc(NULL, sizeof(LootDB));
    memset(db, 0, sizeof(LootDB));
    parse_loot_section(db, sb, NULL);
    sb_free(sb);

    resolve_all_loot_tables(db);

    global_loot_db = db;

    printf_log("Loaded global loot DB: %d groups, %d tables", 
        db->group_count, db->table_count);
}

// Loot exec helpers ///////////////////////////////////////////////////////////

static int weighted_pick_index(LootEntry* entries, int entry_count) 
{
    int total_weight = 0;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].weight > 0)
            total_weight += entries[i].weight;
    }
    if (total_weight <= 0) {
        return -1;
    }

    int r = RNG_RANGE(1, total_weight);
    int cumulative = 0;
    for (int i = 0; i < entry_count; i++) {
        int w = entries[i].weight;
        if (w <= 0)
            continue;
        cumulative += w;
        if (r <= cumulative)
            return i;
    }
    return -1; // Should not reach here
}

static void loot_drop_add(LootDrop* drop, size_t* drop_count, LootType type, 
    VNUM item_vnum, int qty) 
{
    if (qty <= 0)
        return;

    if (*drop_count >= MAX_LOOT_DROPS) {
        bugf("loot_drop_add: exceeded max loot drops (%d)\n",  MAX_LOOT_DROPS);
        return;
    }

    // Merge with existing drop if possible
    for (size_t i = 0; i < *drop_count; i++) {
        if (drop[i].type == type && drop[i].item_vnum == item_vnum) {
            drop[i].qty += qty;
            return;
        }
    }

    drop[*drop_count].type = type;
    drop[*drop_count].item_vnum = item_vnum;
    drop[*drop_count].qty = qty;
    (*drop_count)++;
}

static bool removed_loot_group(const char removed_loot_groups[16][MIL], size_t removed_count, 
    const char* group_name) 
{
    for (size_t i = 0; i < removed_count; i++)
        if (strcmp(removed_loot_groups[i], group_name) == 0)
            return true;
    return false;
}

static bool removed_loot_item(const VNUM* removed_loot_items, int removed_count, 
    VNUM item_vnum) 
{
    for (int i = 0; i < removed_count; i++)
        if (removed_loot_items[i] == item_vnum)
            return true;
    return false;
}

// Generate loot ///////////////////////////////////////////////////////////////

// Generates a list of loot drops for a mob kill.
// Inputs:
//   - table_name: resolved table/profile to use (e.t. mob table already inherits area->global
// Outputs:
//   - drops: output array of loot drops
//   - drops_count: number of loot drops generated
    
void generate_loot(LootDB* db, const char* table_name, 
    LootDrop drops[MAX_LOOT_DROPS], size_t* drop_count)
{
    *drop_count = 0;
    LootTable* table = loot_db_find_table(db, table_name);
    if (!table) {
        if (!test_output_enabled)
            bugf("generate_loot: unknown loot table '%s'\n", table_name);
        return;
    }

    int cp_mul_percent = 100; // Start at 100%, modified by MUL_CP ops
    int chance_mul_percent = 100; // Start at 100%, modified by MUL_ALL_CHANCES ops

    // Keep small "removed" lists to handle REMOVE_ITEM/GROUP ops
    char removed_loot_groups[16][MIL];
    size_t removed_loot_group_count = 0;
    VNUM removed_loot_items[16];
    size_t removed_loot_item_count = 0;

    for (int i = 0; i < table->resolved_op_count; i++) {
        LootOp* op = &table->resolved_ops[i];

        switch (op->type) {

            case LOOT_OP_MUL_CP: {
                if (op->a > 0)
                    cp_mul_percent = (cp_mul_percent * op->a) / 100;
                break;
            }
            case LOOT_OP_MUL_ALL_CHANCES: {
                if (op->a > 0)
                    chance_mul_percent = (chance_mul_percent * op->a) / 100;
                break;
            }
            case LOOT_OP_REMOVE_GROUP: {
                if (removed_loot_group_count < sizeof(removed_loot_groups) / sizeof(removed_loot_groups[0])) {
                    strncpy(removed_loot_groups[removed_loot_group_count++], op->group_name, MIL);
                }
                break;
            }
            case LOOT_OP_REMOVE_ITEM: {
                if (removed_loot_item_count < sizeof(removed_loot_items) / sizeof(removed_loot_items[0])) {
                    removed_loot_items[removed_loot_item_count++] = (VNUM)op->a;
                }
                break;
            }
            case LOOT_OP_USE_GROUP: {
                // Check if group is removed
                if (removed_loot_group(removed_loot_groups, removed_loot_group_count, op->group_name))
                    break;

                int effective_chance = op->a * chance_mul_percent / 100;
                int roll = RNG_RANGE(1, 100);
                if (roll > effective_chance)
                    break; // Did not pass chance roll

                // Find group
                LootGroup* group = loot_db_find_group(db, op->group_name);
                if (!group) {
                    bugf("generate_loot: unknown loot group '%s' in table '%s'\n", 
                        op->group_name, table->name);
                    break;
                }

                if (group->entry_count <= 0)
                    break; // No entries

                // Roll on group
                for (int r = 0; r < group->rolls; r++) {
                    int idx = weighted_pick_index(group->entries, group->entry_count);
                    if (idx < 0)
                        continue; // No valid entries
                    LootEntry* entry = &group->entries[idx];

                    if (entry->type == LOOT_ITEM) {
                        if (removed_loot_item(removed_loot_items, removed_loot_item_count, entry->item_vnum))
                            continue;
                        int qty = RNG_RANGE(entry->min_qty, entry->max_qty);
                        loot_drop_add(drops, drop_count, LOOT_ITEM, entry->item_vnum, qty);
                    }
                    else if (entry->type == LOOT_CP) {
                        int qty = RNG_RANGE(entry->min_qty, entry->max_qty);
                        qty = (qty * cp_mul_percent) / 100;
                        loot_drop_add(drops, drop_count, LOOT_CP, 0, qty);
                    }
                }
                break;
            }
            case LOOT_OP_ADD_ITEM: {
                VNUM vnum = (VNUM)op->a;
                int base_chance = op->b;
                int mn = op->c;
                int mx = op->d;

                if (removed_loot_item(removed_loot_items, removed_loot_item_count, vnum))
                    break;

                int effective_chance = (base_chance * chance_mul_percent) / 100;
                if (RNG_PERCENT() > effective_chance)
                    break;

                int qty = RNG_RANGE(mn, mx);
                loot_drop_add(drops, drop_count, LOOT_ITEM, vnum, qty);
                break;
            }
            case LOOT_OP_ADD_CP: {
                int base_chance = op->a;
                int mn = op->c;
                int mx = op->d;

                int effective_chance = (base_chance * chance_mul_percent) / 100;
                if (RNG_PERCENT() > effective_chance)
                    break;

                int cp = RNG_RANGE(mn, mx);
                cp = (cp * cp_mul_percent) / 100;
                loot_drop_add(drops, drop_count, LOOT_CP, 0, cp);
                break;
            }
            default:
                bugf("generate_loot: unknown loot op type %d in table '%s'\n", 
                    op->type, table->name);
                break;
        }
    }
}

void add_loot_to_container(LootDrop* drops, size_t drop_count, Object* container) 
{
    if (container->item_type != ITEM_CONTAINER) {
        bugf("add_loot_container: object is not a container\n");
        return;
    }

    for (size_t i = 0; i < drop_count; i++) {
        LootDrop* drop = &drops[i];
        if (drop->type == LOOT_ITEM) {
            for (int j = 0; j < drop->qty; j++) {
                ObjPrototype* obj_proto = get_object_prototype(drop->item_vnum);
                if (!obj_proto) {
                    bugf("add_loot_container: unknown object vnum %d\n", 
                        drop->item_vnum);
                    continue;
                }
                Object* obj = create_object(obj_proto, obj_proto->level);
                if (!obj) {
                    bugf("add_loot_container: failed to create object vnum %d\n", 
                        drop->item_vnum);
                    continue;
                }
                obj_to_obj(obj, container);
            }
        }
        else if (drop->type == LOOT_CP) {
            Object* money = create_money(0, 0, drop->qty);
            if (!money) {
                bugf("add_loot_container: failed to create money object\n");
                continue;
            }
            obj_to_obj(money, container);
        }
    }
}

void add_loot_to_mobile(LootDrop* drops, size_t drop_count, Mobile* mob) 
{
    for (size_t i = 0; i < drop_count; i++) {
        LootDrop* drop = &drops[i];
        if (drop->type == LOOT_ITEM) {
            for (int j = 0; j < drop->qty; j++) {
                ObjPrototype* obj_proto = get_object_prototype(drop->item_vnum);
                if (!obj_proto) {
                    bugf("add_loot_container: unknown object vnum %d\n", 
                        drop->item_vnum);
                    continue;
                }
                Object* obj = create_object(obj_proto, obj_proto->level);
                if (!obj) {
                    bugf("add_loot_container: failed to create object vnum %d\n", 
                        drop->item_vnum);
                    continue;
                }
                obj_to_char(obj, mob);
            }
        }
        else if (drop->type == LOOT_CP) {
            Object* money = create_money(0, 0, drop->qty);
            if (!money) {
                bugf("add_loot_mobile: failed to create money object\n");
                continue;
            }
            obj_to_char(money, mob);
        }
    }
}

// Cleanup /////////////////////////////////////////////////////////////////////

void loot_db_free(LootDB* db) 
{
    if (!db)
        return;

    // Free groups
    for (int i = 0; i < db->group_count; i++) {
        LootGroup* group = &db->groups[i];
        free_string(group->name);
        free(group->entries);
    }
    free(db->groups);

    // Free tables
    for (int i = 0; i < db->table_count; i++) {
        LootTable* table = &db->tables[i];
        free_string(table->name);
        free_string(table->parent_name);
        free(table->ops);
        free(table->resolved_ops);
    }
    free(db->tables);
    free(db);
    db = NULL;
}

// Naive implementation ////////////////////////////////////////////////////////

static void print_drops(const LootDrop* out, int n) {
  for (int i = 0; i < n; i++) {
    if (out[i].type == LOOT_CP) {
      printf("  cp: %d\n", out[i].qty);
    } else {
      printf("  item vnum=%d x%d\n", out[i].item_vnum, out[i].qty);
    }
  }
}

// Naive test function
void test_print() 
{
    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    for (int i = 0; i < 10; i++) {
        generate_loot(global_loot_db, "GLOBAL_T1", drops, &drop_count);
        printf("Kill %d (%s):\n", i + 1, "GLOBAL_T1");
        print_drops(drops, drop_count);
    }
}

// Per-area loot support ///////////////////////////////////////////////////////

LootDB* loot_db_new()
{
    LootDB* db = (LootDB*)xrealloc(NULL, sizeof(LootDB));
    memset(db, 0, sizeof(LootDB));
    return db;
}

void load_area_loot(FILE* fp, Entity* owner)
{
    if (!global_loot_db) {
        bug("load_area_loot: global_loot_db not initialized.", 0);
        return;
    }

    // Read loot section until #ENDLOOT
    StringBuffer* sb = sb_new();
    scarf_loot_section(fp, sb);
    parse_loot_section(global_loot_db, sb, owner);
    sb_free(sb);

    // Resolve inheritance after adding new groups/tables
    resolve_all_loot_tables(global_loot_db);
}
