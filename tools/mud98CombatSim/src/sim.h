////////////////////////////////////////////////////////////////////////////////
// sim.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98_COMBAT_SIM_SIM_H
#define MUD98_COMBAT_SIM_SIM_H

#include "sim_config.h"
#include "metrics.h"

#include <entities/area.h>
#include <entities/mob_prototype.h>
#include <entities/room.h>

#include <stddef.h>
#include <stdbool.h>

typedef struct sim_context_t {
    AreaData* area_data;
    RoomData* room_data;
    Area* area;
    Room* room;
    MobPrototype* attacker_proto;
    MobPrototype* defender_proto;
} SimContext;

bool sim_init(SimContext* ctx, const SimConfig* cfg, char* err, size_t err_len);
void sim_shutdown(SimContext* ctx);
bool sim_run(SimContext* ctx, const SimConfig* cfg, SimRunMetrics* out);

#endif // MUD98_COMBAT_SIM_SIM_H
