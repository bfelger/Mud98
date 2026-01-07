# mud98CombatSim

Headless combat simulation tool for Mud98. Runs configurable 1v1 fights and prints summary metrics (Phase 1: damage per tick and time to kill).

## Build

From repo root:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Binary output:

- `bin/Debug/mud98CombatSim` (multi-config)
- `bin/mud98CombatSim` (single-config)

## Run

From repo root (uses `mud98.cfg` and data files):

```sh
./bin/Debug/mud98CombatSim tools/mud98CombatSim/configs/sample.json
```

Optional directory overrides (same as server/test runner):

```sh
./bin/Debug/mud98CombatSim --dir=/path/to/run --area-dir=/path/to/area config.json
```

## Config (JSON)

```json
{
  "seed": 12345,
  "runs": 100,
  "max_ticks": 200,
  "csv_path": "combat_sim.csv",
  "csv_runs_path": "combat_sim_runs.csv",
  "attacker": {
    "name": "Attacker",
    "type": "sim",
    "level": 10,
    "hitpoints": 120,
    "hitroll": 0,
    "armor": 100,
    "damage": { "dice": 1, "size": 8, "bonus": 0 }
  },
  "defender": {
    "name": "Defender",
    "type": "sim",
    "level": 10,
    "hitpoints": 120,
    "hitroll": 0,
    "armor": 100,
    "damage": { "dice": 1, "size": 8, "bonus": 0 }
  }
}
```

Notes:
- If `seed` is omitted, the RNG uses time-based seeding.
- `damage.bonus` maps to the base damroll for the NPC.

## Combatant Types

Each combatant can be one of:
- `sim` (default): uses the configured stats directly (NPC-style).
- `player`: creates a PC with class/race, stats, skills, and a weapon.
- `mob`: loads a live mob by vnum and optionally applies overrides.

Player example:

```json
{
  "attacker": {
    "type": "player",
    "name": "Elithir",
    "class": "warrior",
    "race": "human",
    "level": 1,
    "hitpoints": 120,
    "hitroll": 0,
    "armor": 100,
    "stats": [17, 14, 13, 15, 12],
    "skills": { "sword": 75 },
    "weapon_vnum": 3702
  },
  "defender": {
    "type": "mob",
    "vnum": 3706
  }
}
```

## AI and Spells

AI toggles live under `ai` per combatant:
- `auto_attack`: enable basic combat rounds (default `true`).
- `use_spec`: run NPC `spec_` procedures on the mobile update cadence (default `true`).
- `use_spells`: enable spell casting from the `spells` list (default `false`, auto-true if spells are listed).

Spell lists are arrays of strings or objects:

```json
{
  "attacker": {
    "type": "player",
    "class": "mage",
    "race": "human",
    "level": 12,
    "skills": { "fireball": 75 },
    "ai": { "use_spells": true },
    "spells": [
      "fireball",
      { "name": "magic missile", "cooldown": 2, "chance": 60 }
    ]
  },
  "defender": {
    "type": "mob",
    "vnum": 3706,
    "ai": { "use_spec": true },
    "spec": "spec_cast_mage"
  }
}
```

Notes:
- `spec` expects a full `spec_` name (e.g., `spec_cast_mage`).
- Spell cooldowns are in combat ticks; chance is 0-100.
- Example config: `tools/mud98CombatSim/configs/spell_ai.json`.

## CSV Output

When `csv_path` is set and no sweep is configured, each run is written as one CSV row.
Columns:

- `run`, `ticks`, `seconds`, `total_damage`, `damage_per_tick`
- `attacker_won`, `defender_won`, `timed_out`
- `attack_rolls`, `hits`, `misses`, `parries`, `dodges`, `blocks`
- `spells_cast`, `mana_spent`
- `hit_rate`, `miss_rate`, `parry_rate`, `dodge_rate`, `block_rate`

When a sweep is configured, `csv_path` becomes the per-sweep summary output.
Set `csv_runs_path` if you also want raw per-run rows.

## Sweep (Parameter Variance)

Use `sweep` to vary one stat over a range and report averages per value.

```json
{
  "runs": 200,
  "csv_path": "sweep_summary.csv",
  "csv_runs_path": "sweep_runs.csv",
  "sweep": {
    "target": "defender",
    "stat": "armor",
    "start": 50,
    "end": 150,
    "step": 10
  }
}
```

Supported `sweep.stat` values:
- `armor` (alias: `ac`)
- `level`
- `hitroll`
- `hitpoints` (alias: `hp`)
- `damage_dice`
- `damage_size`
- `damage_bonus`
