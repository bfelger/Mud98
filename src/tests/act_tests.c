////////////////////////////////////////////////////////////////////////////////
// act_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <comm.h>
#include <handler.h>

TestGroup act_tests;

static int test_to_vch()
{
    Room* room = mock_room(50000, NULL, NULL);

    Mobile* ch = mock_mob("Bob", 1001, NULL);
    transfer_mob(ch, room);

    Mobile* pc = mock_player("Jim");
    transfer_mob(pc, room);

    const char* msg = "$n waves at you.";
    act(msg, ch, NULL, pc, TO_VICT);

    ASSERT_OUTPUT_EQ("Bob waves at you.\n\r");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_to_room()
{
    Room* room = mock_room(50000, NULL, NULL);

    Mobile* ch = mock_mob("Bob", 1001, NULL);
    transfer_mob(ch, room);

    Mobile* vch = mock_mob("Alice", 1002, NULL);
    vch->sex = SEX_FEMALE;
    transfer_mob(vch, room);

    Mobile* pc = mock_player("Jim");
    transfer_mob(pc, room);

    const char* msg = "$n looks at $N and tries to steal $S lunch. But $E is "
        "having none of $s shenanigans and bops $m. $n glowers at $M.";
    act(msg, ch, room, vch, TO_ROOM);

    ASSERT_OUTPUT_EQ("Bob looks at Alice and tries to steal her lunch. But "
        "she is having none of his shenanigans and bops him. Bob glowers at "
        "her.\n\r");
    test_output_buffer = NIL_VAL;

    return 0;
}

void register_act_tests()
{
#define REGISTER(n, f)  register_test(&act_tests, (n), (f))

    init_test_group(&act_tests, "ACT TESTS");
    register_test_group(&act_tests);

    REGISTER("Act: To Victim", test_to_vch);
    REGISTER("Act: To Room", test_to_room);

#undef REGISTER
}
