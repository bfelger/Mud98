////////////////////////////////////////////////////////////////////////////////
// tests.c
// Unit test and benchmark functions
////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "merc.h"

#include "benchmark.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "stringutils.h"
#include "tests.h"

#include "entities/mobile.h"

#include "data/mobile_data.h"

#include "lox/lox.h"

extern void aggr_update();

static void old_aggr_update()
{
    Mobile* wch;
    Mobile* wch_next = NULL;
    Mobile* ch;
    Mobile* vch;
    Mobile* victim;

    for (wch = mob_list; wch != NULL; wch = wch_next) {
        wch_next = wch->next;
        if (IS_NPC(wch) || wch->level >= LEVEL_IMMORTAL || wch->in_room == NULL
            || wch->in_room->area->empty)
            continue;

        FOR_EACH_ROOM_MOB(ch, wch->in_room) {
            int count;

            if (!IS_NPC(ch) || !IS_SET(ch->act_flags, ACT_AGGRESSIVE)
                || IS_SET(ch->in_room->data->room_flags, ROOM_SAFE)
                || IS_AFFECTED(ch, AFF_CALM) || ch->fighting != NULL
                || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch)
                || (IS_SET(ch->act_flags, ACT_WIMPY) && IS_AWAKE(wch))
                || !can_see(ch, wch) || number_bits(1) == 0)
                continue;

            count = 0;
            victim = NULL;
            FOR_EACH_ROOM_MOB(vch, wch->in_room) {
                if (!IS_NPC(vch) && vch->level < LEVEL_IMMORTAL
                    && ch->level >= vch->level - 5
                    && (!IS_SET(ch->act_flags, ACT_WIMPY) || !IS_AWAKE(vch))
                    && can_see(ch, vch)) {
                    if (number_range(0, count) == 0)
                        victim = vch;
                    count++;
                }
            }

            if (victim == NULL)
                continue;

            multi_hit(ch, victim, TYPE_UNDEFINED);
        }
    }

    return;
}

static void aggr_update_bnchmk()
{
    static int num_iters = 100000;

    printf("\n");
    printf("aggr_udpate benchmark:\n");
    printf("old:     ");

    Timer timer = { 0 };
    start_timer(&timer);
    for (int i = 0; i < num_iters; ++i)
        old_aggr_update();
    stop_timer(&timer);
    struct timespec timer_res = elapsed(&timer);

    printf(TIME_FMT"s, %ldms\n", timer_res.tv_sec, timer_res.tv_nsec / 1000000);

    printf("new:     ");

    reset_timer(&timer);
    start_timer(&timer);
    for (int i = 0; i < num_iters; ++i)
        aggr_update();
    stop_timer(&timer);
    timer_res = elapsed(&timer);

    printf(TIME_FMT"s, %ldms\n", timer_res.tv_sec, timer_res.tv_nsec / 1000000);
}

void run_unit_tests()
{
    printf("Running unit tests and benchmarks:\n\n");

    //aggr_update_bnchmk();

    //char* source =
    //    "fun outer() {\n"
    //    "  var x = \"outside\";\n"
    //    "  fun inner() {\n"
    //    "    print x;\n"
    //    "  }\n"
    //    "\n"
    //    "  return inner;\n"
    //    "}\n"
    //    "var closure = outer();\n"
    //    "closure();\n";

    //char* source =
    //    "fun fib(n) {\n"
    //    "  if (n < 2) return n;\n"
    //    "  return fib(n - 2) + fib(n - 1);\n"
    //    "}\n"
    //    "var start = clock();\n"
    //    "print fib(20);\n"
    //    "print clock() - start;\n";

    //char* source =
    //    "class Pair {}\n"
    //    "var pair = Pair();\n"
    //    "pair.first = 1;\n"
    //    "pair.second = 2;\n"
    //    "print pair.first + pair.second; // 3.\n";

    //char* source =
    //    "class CoffeeMaker {\n"
    //    "  init(coffee) {\n"
    //    "    this.coffee = coffee;\n"
    //    "  }\n"
    //    "  brew() {\n"
    //    "    print \"Enjoy your cup of \" + this.coffee;\n"
    //    "\n"
    //    "    // No reusing the grounds!\n"
    //    "    this.coffee = nil;\n"
    //    "  }\n"
    //    "}\n"
    //    "var maker = CoffeeMaker(\"coffee and chicory\");\n"
    //    "maker.brew();\n";

    //char* source =
    //    "class Doughnut {\n"
    //    "  cook() {\n"
    //    "    print \"Dunk in the fryer.\";\n"
    //    "  }\n"
    //    "}\n"
    //    "class Cruller < Doughnut {\n"
    //    "  finish() {\n"
    //    "    print \"Glaze with icing.\";\n"
    //    "  }\n"
    //    "}\n"
    //    "var a = Cruller();\n"
    //    "a.cook();\n"
    //    "a.finish();\n";

    //char* source =
    //    "class Zoo {\n"
    //    "    init()\n"
    //    "    {\n"
    //    "        this.aardvark = 1;\n"
    //    "        this.baboon = 1;\n"
    //    "        this.cat = 1;\n"
    //    "        this.donkey = 1;\n"
    //    "        this.elephant = 1;\n"
    //    "        this.fox = 1;\n"
    //    "    }\n"
    //    "    ant() { return this.aardvark; }\n"
    //    "    banana() { return this.baboon; }\n"
    //    "    tuna() { return this.cat; }\n"
    //    "    hay() { return this.donkey; }\n"
    //    "    grass() { return this.elephant; }\n"
    //    "    mouse() { return this.fox; }\n"
    //    "}\n"
    //    "\n"
    //    "var zoo = Zoo();\n"
    //    "var sum = 0;\n"
    //    "var start = clock();\n"
    //    "while (sum < 100000000) {\n"
    //    "    sum = sum + zoo.ant()\n"
    //    "        + zoo.banana()\n"
    //    "        + zoo.tuna()\n"
    //    "        + zoo.hay()\n"
    //    "        + zoo.grass()\n"
    //    "        + zoo.mouse();\n"
    //    "}\n"
    //    "\n"
    //    "print clock() - start;\n"
    //    "print sum;\n";

    //InterpretResult result = interpret_code(source);

    //char* source =
    //    "var a = [0, 1, 2, 3, 5, 8, 13, 21];\n"
    //    "a[2] = 100;\n"
    //    "for (var i = 0; i < 8; i += 2)\n"
    //    "   print a[i];\n";

    //int16_t i16 = 16;
    //int32_t i32 = 32;
    //uint64_t u64 = 64;
    //char* str = str_dup("string");
    //
    //char* source =
    //    "fun test_interop(i16, i32, u64, str) {\n"
    //    "   print i16;\n"
    //    "   print i32;\n"
    //    "   print u64;\n"
    //    "   print str;\n"
    //    "   i16++;\n"
    //    "   i32--;\n"
    //    "   u64++;\n"
    //    "   str = \"blah blah blah\";\n"
    //    "}\n";
    //
    //InterpretResult result = interpret_code(source);
    //
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    //
    //ObjArray* args = new_obj_array();
    //push(OBJ_VAL(&args)); // Protect args as we make them.
    //
    //Value raw_i16 = WRAP_I16(i16);
    //write_value_array(&args->val_array, raw_i16);
    //
    //Value raw_i32 = WRAP_I32(i32);
    //write_value_array(&args->val_array, raw_i32);
    //
    //Value raw_u64 = WRAP_U64(u64);
    //write_value_array(&args->val_array, raw_u64);
    //
    //Value raw_str = WRAP_STR(str);
    //write_value_array(&args->val_array, raw_str);
    //
    //result = call_function("test_interop", 4, raw_i16, raw_i32, raw_u64, raw_str);
    //
    //result = call_function("test_interop", 4, raw_i16, raw_i32, raw_u64, raw_str);
    //
    //pop();

    //char* source = 
    //    "fun print_room(room) {\n"
    //    "   print room.name();\n"
    //    "   print room.vnum();\n"
    //    "}\n";

    //char* source = 
    //    "print [0, 1, 2, 3, 4, 5].count;"
    //    "fun print_mob(mob) {\n"
    //    "   var contents = mob.carrying();\n"
    //    "   if (contents.count > 0) {\n"
    //    "       var first = contents[0];\n"
    //    "       var str = \" carrying \" + contents[0].short_desc();\n"
    //    "       for (var i = 1; i < contents.count; i++)\n"
    //    "           str += \", \" + contents[i].short_desc();"
    //    "       print string(mob.vnum()) + \" \" + mob.short_desc() + str; "
    //    "   }\n"
    //    "   else\n"
    //    "       print string(mob.vnum()) + \" \" + mob.short_desc();\n"
    //    "}\n";
    //
    //InterpretResult result = interpret_code(source);
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    //

    //Room* room;
    //Mobile* mob;
    //int count = 0;
    //for (int i = 0; i < top_vnum_room; ++i) {
    //    if ((room = get_room(NULL, i)) != NULL) {
    //        FOR_EACH_IN_ROOM(mob, room->people) {
    //            if (count++ == 100)
    //                goto loop_end;
    //            Value mob_val = create_mobile_value(mob);
    //            push(mob_val);
    //            result = call_function("print_mob", 1, mob_val);
    //            pop();
    //            if (result != INTERPRET_OK) goto loop_end;
    //        }
    //    }
    //}
    //loop_end:

   //char* source =
   //    "var i = 0;"
   //    "for (; i < 100; i++) {\n"
   //    "   if (floor((i + 1) / 2) == i / 2)\n"
   //    "       continue;\n"
   //    "   else if (i > 50)\n"
   //    "       break;\n"
   //    "   print i;\n"
   //    "}\n";

    //char* source =
    //    "print Damage.lightning;";

    //char* source = 
    //    "fun print_name(thing) {\n"
    //    "   print \"[\" + string(thing.vnum) + \"] \" + thing.name;\n"
    //    "}\n";

    //char* source =
    //    "for (var i = 0; i < global_areas.count; i++) {\n"
    //    "    var area = global_areas[i];\n"
    //    "    print area.name;\n"
    //    "    if (area.instances.count > 0) {\n"
    //    "        var inst = area.instances[0];\n"
    //    "        print inst.rooms.count;\n"
    //    "    }\n"
    //    "}\n"
    //    "print floor(2.5);";

    char* source =
    //"var lam = (index, area) -> {\n"
    ////"    print string(index) + \" \" + area.name;\n"
    //"    print index;\n"
    //"    print area.name;\n"
    //"};\n"
    //"fun lam2(index, area) {\n"
    //"    print index;\n"
    //"    print area.name;\n"
    //"};\n"
    //"for (var i = 0; i < global_areas.count; i++) {\n"
    //"    var area = global_areas[i];\n"
    //"    lam(i, area);\n"
    //"}\n";
    //"global_areas.apply(lam);\n";
    //"global_areas.apply((index, area) -> {\n"
    //"    print index;\n"
    //"    print area.name;\n"
    //"});\n";
    "global_areas[3].instances[0].rooms.apply((key, room) -> {\n"
    "    print key + \" \" + room.name;\n"
    "    room.mobiles.apply((mob) -> {\n"
    "        print \"    \" + mob.name + \" (\" + mob.vnum + \")\";\n"
    "    });\n"
    "});\n";

    InterpretResult result = interpret_code(source);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    
    //Room* room;
    //int count = 0;
    //for (int i = 0; i < top_vnum_room; ++i) {
    //    if ((room = get_room(NULL, i)) != NULL) {
    //        if (count++ == 100)
    //            goto loop_end;
    //        Value room_val = OBJ_VAL(room);
    //        push(room_val);
    //        result = call_function("print_name", 1, room_val);
    //        pop();
    //        if (result != INTERPRET_OK) goto loop_end;
    //    }
    //}
    //loop_end:

    //Object* obj = obj_list;
    //
    //for (int i = 0; i < 100; i++) {
    //    Value obj_val = OBJ_VAL(obj);
    //    push(obj_val);
    //    result = call_function("print_name", 1, obj_val);
    //    pop();
    //    if (result != INTERPRET_OK) 
    //        break;
    //    obj = obj->next;
    //}

    //Mobile* mob = mob_list;
    //
    //for (int i = 0; i < 100; i++) {
    //    Value mob_val = OBJ_VAL(mob);
    //    push(mob_val);
    //    result = call_function("print_name", 1, mob_val);
    //    pop();
    //    if (result != INTERPRET_OK)
    //        break;
    //    mob = mob->next;
    //}
    
    //InterpretResult result = interpret_code(source);
    //result = interpret_code(source);
    //
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    //printf("i16 = %d\ni32 = %d\nu64 = %llu\nstr = '%s'\n", i16, i32, u64, str);

    printf("\nAll tests and benchmarks complete.\n");
}

