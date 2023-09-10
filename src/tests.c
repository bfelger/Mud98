////////////////////////////////////////////////////////////////////////////////
// tests.c
// Unit test and benchmark functions
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "benchmark.h"
#include "strings.h"
#include "tests.h"

#include "entities/char_data.h"

extern void aggr_update();

static void old_aggr_update()
{
    CharData* wch;
    CharData* wch_next = NULL;
    CharData* ch;
    CharData* ch_next = NULL;
    CharData* vch;
    CharData* vch_next = NULL;
    CharData* victim;

    for (wch = char_list; wch != NULL; wch = wch_next) {
        wch_next = wch->next;
        if (IS_NPC(wch) || wch->level >= LEVEL_IMMORTAL || wch->in_room == NULL
            || wch->in_room->area->empty)
            continue;

        for (ch = wch->in_room->people; ch != NULL; ch = ch_next) {
            int count;

            ch_next = ch->next_in_room;

            if (!IS_NPC(ch) || !IS_SET(ch->act, ACT_AGGRESSIVE)
                || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
                || IS_AFFECTED(ch, AFF_CALM) || ch->fighting != NULL
                || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch)
                || (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
                || !can_see(ch, wch) || number_bits(1) == 0)
                continue;

            count = 0;
            victim = NULL;
            for (vch = wch->in_room->people; vch != NULL; vch = vch_next) {
                vch_next = vch->next_in_room;

                if (!IS_NPC(vch) && vch->level < LEVEL_IMMORTAL
                    && ch->level >= vch->level - 5
                    && (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch))
                    && can_see(ch, vch)) {
                    if (number_range(0, count) == 0) victim = vch;
                    count++;
                }
            }

            if (victim == NULL) continue;

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
    printf("Running unit tests and benchmarks:\n");

    aggr_update_bnchmk();

    printf("All tests and benchmarks complete.\n");
}

