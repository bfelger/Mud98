////////////////////////////////////////////////////////////////////////////////
// tests.c
// Unit test and benchmark functions
////////////////////////////////////////////////////////////////////////////////

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

#include "lox/chunk.h"
#include "lox/debug.h"
#include "lox/vm.h"

extern void aggr_update();

static void old_aggr_update()
{
    Mobile* wch;
    Mobile* wch_next = NULL;
    Mobile* ch;
    Mobile* ch_next = NULL;
    Mobile* vch;
    Mobile* vch_next = NULL;
    Mobile* victim;

    for (wch = mob_list; wch != NULL; wch = wch_next) {
        wch_next = wch->next;
        if (IS_NPC(wch) || wch->level >= LEVEL_IMMORTAL || wch->in_room == NULL
            || wch->in_room->area->empty)
            continue;

        for (ch = wch->in_room->people; ch != NULL; ch = ch_next) {
            int count;

            ch_next = ch->next_in_room;

            if (!IS_NPC(ch) || !IS_SET(ch->act_flags, ACT_AGGRESSIVE)
                || IS_SET(ch->in_room->data->room_flags, ROOM_SAFE)
                || IS_AFFECTED(ch, AFF_CALM) || ch->fighting != NULL
                || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch)
                || (IS_SET(ch->act_flags, ACT_WIMPY) && IS_AWAKE(wch))
                || !can_see(ch, wch) || number_bits(1) == 0)
                continue;

            count = 0;
            victim = NULL;
            for (vch = wch->in_room->people; vch != NULL; vch = vch_next) {
                vch_next = vch->next_in_room;

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

    init_vm();

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

    char* source =
        "var a = [0, 1, 2, 3, 5, 8, 13, 21];\n"
        "a[2] = 100;\n"
        "var i;\n"
        "for (i = 0; i < 8; i = i + 1)\n"
        "   print a[i];\n";

    InterpretResult result = interpret_code(source);

    free_vm();

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);

    printf("\nAll tests and benchmarks complete.\n");
}

