////////////////////////////////////////////////////////////////////////////////
// container_benchmarks.c
////////////////////////////////////////////////////////////////////////////////

#include "benchmarks.h"

#include <db.h>
#include <stringutils.h>

#include <entities/entity.h>

#include <lox/array.h>
#include <lox/list.h>
#include <lox/segvec.h>
#include <lox/table.h>

#define ITERATIONS 100000

#if __GNUC__ || __clang__
#  define UNUSED __attribute__((unused))
#else
#  define UNUSED /**/
#endif

typedef struct Item {
    Entity header;
    int num;
    Node* item_list_node;
    struct Item* next;
} Item;

Item* slab = NULL;
int* search_pattern;

Item* item_list = NULL;
size_t item_count = 0;
size_t item_perm_count = 0;

List item_lox_list = { 0 };
SegmentedVector item_segvec = { 0 };
ValueArray item_lox_array = { 0 };
Table item_table = { 0 };

#ifdef COUNT_SIZE_ALLOCS
#define START_TEST() amt_perm_alloced = 0; amt_temp_alloced = 0; amt_temp_freed = 0;
#define END_TEST() \
    printf(", alloced: %5llu perm, %8llu temp, %7llu freed.\n", amt_perm_alloced, amt_temp_alloced, amt_temp_freed);
#else
#define START_TEST()    
#define END_TEST() \
    printf(".\n");
#endif

#define START_TIMER() \
    reset_timer(&timer); \
    start_timer(&timer);
#define END_TIMER() end_timer(&timer);

static inline void end_timer(Timer* timer)
{
    stop_timer(timer);
    struct timespec timer_res = elapsed(timer);
    printf("%10ldns", timer_res.tv_nsec);
}

static inline Item* pop_item_raw_list()
{
    if (item_list == NULL)
        return NULL;

    Item* item = item_list;
    item_list = item_list->next;
    item_count--;
    return item;
}

static void benchmark_linked_list(const char* msg)
{
    printf("%s", msg);
    START_TEST()

    Timer timer = { 0 };
    Item* item;

    printf("Adding %d items:   ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        item = &slab[i];
        item_count++;
        item->next = item_list;
        item_list = item;
    }
    END_TIMER();
    END_TEST();

    START_TEST();
    printf("            Indexing %d items: ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        int n = search_pattern[i];
        int o = ITERATIONS - n - 1;
        item = item_list;
        for (int j = 0; j < n; j++) {
            item = item->next;
        }
        if (item->num != o) {
            printf("benchmark_raw_list: list[%d] -> Expected %d, got %d.\n", n, o, item->num);
            exit(1);
        }
    }
    END_TIMER();
    printf("\n");

    START_TEST();
    printf("            Freeing %d items:  ", ITERATIONS);
    START_TIMER();
    while ((item = pop_item_raw_list()) != NULL) { 
    }
    END_TIMER();
    END_TEST()
}

static inline void persist_item_lox_list(Item* item)
{
    item->item_list_node = list_push(&item_lox_list, OBJ_VAL(item));
}

static inline Item* pop_item_lox_list()
{
    if (item_lox_list.count == 0)
        return NULL;

    Item* item = (Item*)AS_ENTITY(item_lox_list.front->value);
    list_remove_node(&item_lox_list, item->item_list_node); 
    return item;
}

static inline Item* index_lox_list(int idx)
{
    Node* node = item_lox_list.front;
    for (int i = 0; i < idx; i++) {
        node = node->next;
    }
    return (Item*)AS_ENTITY(node->value);
}

static void benchmark_lox_list(const char* msg)
{
    printf("%s", msg);
    START_TEST()
    Timer timer = { 0 };
    Item* item;

    printf("Adding %d items:   ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) { 
        item = &slab[i];
        persist_item_lox_list(item);
    }
    END_TIMER();
    END_TEST();

    START_TEST();
    printf("            Indexing %d items: ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        int n = search_pattern[i];
        int o = ITERATIONS - n - 1;
        item = index_lox_list(n);
        if (item->num != o) {
            printf("benchmark_lox_list: list[%d] -> Expected %d, got %d.\n", n, o, item->num);
            exit(1);
        }
    }
    END_TIMER();
    printf("\n");

    START_TEST();
    printf("            Freeing %d items:  ", ITERATIONS);
    START_TIMER();
    while ((item = pop_item_lox_list()) != NULL) {
    }
    END_TIMER();
    END_TEST()
}

static inline void persist_item_lox_array(Item* item)
{
    write_value_array(&item_lox_array, OBJ_VAL(item));
}

static inline Item* pop_item_lox_array()
{
    if (item_lox_array.count == 0)
        return NULL;

    Value val = item_lox_array.values[item_lox_array.count - 1];
    Item* item = (Item*)AS_ENTITY(val);
    remove_array_index(&item_lox_array, item_lox_array.count - 1);
    return item;
}

void benchmark_lox_array(const char* msg)
{
    printf("%s", msg);
    START_TEST()
    Timer timer = { 0 };
    Item* item;

    const int array_iters = ITERATIONS / 10;

    printf("Adding %d items:   ", array_iters * 10);

    START_TIMER();
    for (int i = 0; i < array_iters; i++) {
        item = &slab[i];

        persist_item_lox_array(item);
    }
 
    stop_timer(&timer);
    struct timespec timer_res = elapsed(&timer);
    printf("%10ldns", timer_res.tv_nsec * 10);
#ifdef COUNT_SIZE_ALLOCS
    printf(", alloced: %5llu perm, %8llu temp, %7llu freed.\n", 
        amt_perm_alloced * 10, amt_temp_alloced * 10, amt_temp_freed * 10);
#else
    printf(".\n");
#endif

    START_TEST();
    printf("            Indexing %d items: ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < array_iters; i++) {
        int n = search_pattern[i];
        int o = n; // array_iters - n - 1;
        Value val = item_lox_array.values[n];
        item = (Item*)AS_ENTITY(val);
        if (item->num != o) {
            printf("benchmark_lox_array: array[%d] -> Expected %d, got %d.\n", n, o, item->num);
            exit(1);
        }
    }
    stop_timer(&timer);
    timer_res = elapsed(&timer);
    printf("%10ldns", timer_res.tv_nsec * 10);
    printf("\n");

    START_TEST()
    printf("            Freeing %d items:  ", array_iters * 10);
    START_TIMER();
    while ((item = pop_item_lox_array()) != NULL) {
    }

    stop_timer(&timer);
    timer_res = elapsed(&timer);
    printf("%10ldns", timer_res.tv_nsec * 10);
#ifdef COUNT_SIZE_ALLOCS
    printf(", alloced: %5llu perm, %8llu temp, %7llu freed.\n",
        amt_perm_alloced * 10, amt_temp_alloced * 10, amt_temp_freed * 10);
#else
    printf(".\n");
#endif
}

static void benchmark_segvec(const char* msg)
{
    printf("%s", msg);
    START_TEST()
    Timer timer = { 0 };

    printf("Adding %d items:   ", ITERATIONS);

    Item* item;

    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        item = &slab[i];

        Value val = OBJ_VAL(item);
        if (!sv_push(&item_segvec, val)) {
            fprintf(stderr, "Failed to push to item_segvec\n");
            exit(1);
        }
    }
    END_TIMER();
    END_TEST();

    START_TEST();
    printf("            Indexing %d items: ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        int n = search_pattern[i];
        int o = n;
        Value* val = sv_at(&item_segvec, n);
        item = (Item*)AS_ENTITY(*val);
        if (item->num != o) {
            printf("benchmark_segvec: sv[%d] -> Expected %d, got %d.\n", n, o, item->num);
            exit(1);
        }
    }
    END_TIMER();
    printf("\n");

    START_TEST()
    printf("            Freeing %d items:  ", ITERATIONS);
    START_TIMER();
    while (item_segvec.count > 0) {
        Value val;
        if (!sv_pop(&item_segvec, &val)) {
            fprintf(stderr, "Failed to pop from item_segvec\n");
            exit(1);
        }
    }
    END_TIMER();
    END_TEST()
}

UNUSED
static void benchmark_table(const char* msg)
{
    printf("%s", msg);
    START_TEST()
        Timer timer = { 0 };

    printf("Adding %d items:   ", ITERATIONS);

    Item* item;

    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        item = &slab[i];

        Value val = OBJ_VAL(item);
        if (!table_set_vnum(&item_table, i, val)) {
            fprintf(stderr, "Failed to push to item_table\n");
            exit(1);
        }
    }
    END_TIMER();
    END_TEST();

    START_TEST();
    printf("            Indexing %d items: ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        int n = search_pattern[i];
        int o = n;
        Value val;
        if (!table_get_vnum(&item_table, i, &val)) {
            fprintf(stderr, "Failed to get from item_table\n");
            exit(1);
        }
        item = (Item*)AS_ENTITY(val);
        if (item->num != o) {
            printf("benchmark_table: sv[%d] -> Expected %d, got %d.\n", n, o, item->num);
            exit(1);
        }
    }
    END_TIMER();
    printf("\n");

    START_TEST();
    printf("            Freeing %d items:  ", ITERATIONS);
    START_TIMER();
    for (int i = 0; i < ITERATIONS; i++) {
        if (!table_delete_vnum(&item_table, i)) {
            fprintf(stderr, "Failed to delete from item_table\n");
            exit(1);
        }
    }
    END_TIMER();
    END_TEST();
}

void randomize_search_pattern(int lo, int hi)
{
    // To accomodate the abbreviated ValueArray tests, we make allow the first
    // ITERATIONS/10 items be randomized exclusively.
    // This probably won't have very good entropy; as long as all tests use the
    // same exact search pattern, it should still give us a ball-park of each
    // container's random access performance.
    for (int i = lo; i <= hi; i++) {
        int n = number_range(lo, hi);
        while (search_pattern[n] != -1) {
            n++;
            if (n > hi)
                n = lo;
        }
        search_pattern[n] = i;
    }
}

void benchmark_containers()
{
    const size_t slab_size = sizeof(Item) * ITERATIONS;
    slab = (Item*)malloc(slab_size);
    if (slab == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }
    memset(slab, 0, slab_size);
    for (int i = 0; i < ITERATIONS; i++) {
        slab[i].num = i;
    }

    const size_t search_pattern_size = sizeof(int) * ITERATIONS;
    search_pattern = (int*)malloc(search_pattern_size);
    if (search_pattern == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }
    memset(search_pattern, 0, search_pattern_size);
    for (int i = 0; i < ITERATIONS; i++)
        search_pattern[i] = -1;
    randomize_search_pattern(0, (ITERATIONS / 10) - 1);
    randomize_search_pattern(ITERATIONS / 10, ITERATIONS - 1);

    sv_init(&item_segvec, 4096);

    push(OBJ_VAL(&item_lox_list));
    init_list(&item_lox_list);

    push(OBJ_VAL(&item_lox_array));
    init_value_array(&item_lox_array);

    push(OBJ_VAL(&item_table));
    init_table(&item_table);

    printf("Container Benchmarks:\n");
    benchmark_linked_list("  Raw List: ");
    benchmark_linked_list("    (rerun) ");
    printf("\n");
    benchmark_lox_list("  Lox List: ");
    benchmark_linked_list("    (rerun) ");
    printf("\n");
    benchmark_lox_array(" Lox Array: ");
    benchmark_lox_array("    (rerun) ");
    printf("\n");
    benchmark_segvec(" Lox Table: ");
    benchmark_segvec("    (rerun) ");
    printf("\n");
    benchmark_segvec(" SegVector: ");
    benchmark_segvec("    (rerun) ");
}
