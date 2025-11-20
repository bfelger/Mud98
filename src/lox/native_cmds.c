////////////////////////////////////////////////////////////////////////////////
// native_cmds.c
// Expose game commands to in-game Mobs via Lox scripts
////////////////////////////////////////////////////////////////////////////////

#include "native.h"

#include "table.h"
#include "vm.h"

#include <interp.h>
#include <mob_cmds.h>

Table native_cmds;
Table native_mob_cmds;

void init_native_plr_cmds();
void init_native_mob_cmds();

void init_native_cmds()
{
    init_native_plr_cmds();
    init_native_mob_cmds();
}

static void define_native_cmd(Table* native_cmd_table, const char* name, DoFunc* cmd)
{
    push(OBJ_VAL(copy_string(name, (int)strlen(name))));
    push(OBJ_VAL(new_native_cmd(cmd)));
    table_set(native_cmd_table, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void init_native_plr_cmds()
{
    init_table(&native_cmds);

    for (int i = 0; cmd_table[i].name[0] != '\0'; i++) {
        const CmdInfo* cmd = &cmd_table[i];
        define_native_cmd(&native_cmds, cmd->name, cmd->do_fun);
    }
}

void init_native_mob_cmds()
{
    init_table(&native_mob_cmds);

    for (int i = 0; mob_cmd_table[i].name[0] != '\0'; i++) {
       const MobCmdInfo* cmd = &mob_cmd_table[i];
        define_native_cmd(&native_mob_cmds, cmd->name, cmd->do_fun);
    }
}