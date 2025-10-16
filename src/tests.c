////////////////////////////////////////////////////////////////////////////////
// tests.c
// Unit test and benchmark functions
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "benchmark.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "stringutils.h"
#include "tests.h"

#include <entities/mobile.h>
#include <entities/room.h>
#include <entities/area.h>

#include <data/mobile_data.h>

#include <lox/lox.h>
#include <lox/memory.h>
#include <lox/function.h>
#include <lox/vm.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "tests.h"

void test_room_script_binding();
void test_namespace();

// Implicitly imported from lox/vm.c
bool invoke(ObjString* name, int arg_count);

void run_unit_tests2()
{
    printf("Running unit tests and benchmarks:\n\n");

#ifdef BLAH
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

    //char* source =
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
    //"global_areas.map(lam);\n";
    //"global_areas.each((index, area) -> {\n"
    //"    print index;\n"
    //"    print area.name;\n"
    //"});\n";
    //"var mob_count = 0\n"
    //"global_mobs.each((mob) -> {\n"
    //"    mob_count++\n"
    //"})\n"
    //"print \"Mobile Instance Count: \" + mob_count\n";
    //
    //InterpretResult result = interpret_code(source);
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    
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

    //void spell_acid_blast(SKNUM sn, LEVEL level, Mobile * ch, void* vo, SpellTarget target)
    //{
    //    Mobile* victim = (Mobile*)vo;
    //    int dam;
    //
    //    dam = dice(level, 12);
    //    if (saves_spell(level, victim, DAM_ACID)) dam /= 2;
    //    damage(ch, victim, dam, sn, DAM_ACID, true);
    //    return;
    //}

    //char* source =
    //    "fun spell_fire_bolt(sn, level, ch, victim)\n"
    //    "{\n"
    //    "    var dam = dice(level, 10);\n"
    //    "    if (saves_spell(level, victim, DamageType.Fire))\n"
    //    "        dam = 0;\n"
    //    "    damage(ch, victim, dam, sn, DamageType.Fire, true);\n"
    //    "}\n";
    //
    //InterpretResult result = interpret_code(source);
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    //char* source =
    //"var area = global_areas[0];\n"
    //"if (area.is_area())\n"
    //"    print \"Is an area!\";\n"
    //"else\n"
    //"    print \"Is NOT an area!\";\n";
    //
    //InterpretResult result = interpret_code(source);
    //if (result == INTERPRET_COMPILE_ERROR) exit(65);
    //if (result == INTERPRET_RUNTIME_ERROR) exit(70);
#endif
    test_room_script_binding();
    //test_namespace();

    printf("\nAll tests and benchmarks complete.\n");   
}
    
void test_room_script_binding()
{
    // Mock AreaData
    AreaData* ad = new_area_data();
    push(OBJ_VAL(ad));

    // Mock RoomData
    RoomData* rd = new_room_data();
    push(OBJ_VAL(rd));

    rd->area_data = ad;
    VNUM_FIELD(rd) = 1000;
    table_set_vnum(&global_rooms, VNUM_FIELD(rd), OBJ_VAL(rd));

    const char* src =
        "on_enter() { print \"room entry\"; }"
        "on_exit() { print \"room exit\"; }";

    ObjClass* room_class = create_entity_class("test_room", src);
    rd->header.klass = room_class;

    // Mock Area
    Area* area = create_area_instance(ad, false);
    push(OBJ_VAL(area));

    // Mock Room
    Room* r = get_room(area, VNUM_FIELD(rd));
    push(OBJ_VAL(r));

    // Invoke on_enter
    bool inv_result = invoke(lox_string("on_enter"), 0);

    if (!inv_result) {
        bug("test_room_script_binding: invoke on_enter failed.");
        return;
    }

    InterpretResult result = run();
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    pop(); // room
    pop(); // area
    pop(); // room data
    pop(); // area data

//
//    const char* src = "fun on_enter() { print \"room entry\"; }";
//    ObjFunction* wrapper = NULL;
//    ObjFunction* fn = lox_compile_with_wrapper(src, "on_enter", &wrapper);
//    if (!fn) {
//        bug("test_room_script_binding: compile failed.");
//        pop();
//        return;
//    }
//
//    rd->lox_functions = ALLOCATE(ObjFunction*, 1);
//    rd->lox_functions[0] = fn;
//    rd->lox_function_count = 1;
//
//    /*
//        * Create a minimal AreaData and Area instance so new_room can safely
//        * attach the room into an Area without dereferencing a NULL pointer.
//        */
//    AreaData* ad = new_area_data();
//    rd->area_data = ad;
//    /* assign a VNUM for the prototype and register it in global_rooms */
//    VNUM_FIELD(rd) = 1000;
//    table_set_vnum(&global_rooms, VNUM_FIELD(rd), OBJ_VAL(rd));
//
//    Area* area = create_area_instance(ad, false);
//    Room* r = get_room(area, VNUM_FIELD(rd));
//
//    Value v;
//    if (table_get(&r->lox_instance->fields, fn->name, &v) && IS_CLOSURE(v)) {
//        ObjClosure* clos = AS_CLOSURE(v);
//        invoke_closure(clos, 0);
//    }
//
//    free_room(r);
//    free_room_data(rd);
//    pop();
}

void test_namespace()
{
//    const char* src =
//    "namespace A {\n"
//    "  var x = \"namespace A x\";\n"
//    "  fun print_x() {\n"
//    "    print x;\n"
//    "  }\n"
//    "}\n"
//    "namespace B {\n"
//    "  var x = \"namespace B x\";\n"
//    "  fun print_x() {\n"
//    "    print x;\n"
//    "  }\n"
//    "}\n"
//    "A.print_x();\n"
//    "B.print_x();\n";
//    InterpretResult result = interpret_code(src);
//    if (result == INTERPRET_COMPILE_ERROR) exit(65);
//    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

