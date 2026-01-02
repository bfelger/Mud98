# Loot Drops

Loot is rolled on tables composed of inheritable loot groups.

## Loot Groups

A reusable weighted list of entries. Each entry is either:
- An item VNUM with a quantity range, or
- A currency range (in copper pieces)

## Loot Tables

Loot table is an aggregation of Loot Groups, with tailored alterations:
- Add Items
- Remove items
- Add Currency
- Currency multipliers
- Chance multipliers

Tables are inheritable, so you can have per-area loot tables that build off of global loot tables.

## What it looks like

Here's a rough overview of what some `loot.olc` entries look like:
```
# group <id> <rolls>
group COINS_T1 1
  cp  80 160  weight 100

group GLOBAL_COMMON_T1 1
  item 1201 1 1  weight 60
  item 1202 1 1  weight 30
  item 1203 1 1  weight 10

group GLOBAL_RARE_T1 1
  item 7777 1 1  weight 100


table GLOBAL_T1
  use_group COINS_T3 100
  use_group CONS_COMMON_T3 35
  use_group RARE_GLOBAL_T3 1
```

Here is what a `#LOOT` section in an area file would look like:

```
#LOOT

group AREA_MATS_GLOOMCAVES 2
  item 3401 1 2  weight 70
  item 3402 1 1  weight 30

group BOSS_CHEST_T1 1
  item 8001 1 1  weight 100

table AREA_GLOOMCAVES_T1 parent GLOBAL_T1
  use_group AREA_MATS_GLOOMCAVES 50
  mul_cp 110

table MOB_BAT_SWARM parent AREA_GLOOMCAVES_T1
  add_item 3402 60 1 1

table MOB_FUNGUS_KING parent AREA_GLOOMCAVES_T1
  add_item 9001 100 1 1
  use_group BOSS_CHEST_T1 100
  mul_all_chances 150

#ENDLOOT
```
 JSON representations will be different, of course.

 ## Crafting Loot Tables

 Because loot groups and loot tables are created and referenced in a completely builder-defined, ad hoc manner, the following conventions are completely self-enforced. If you have a different vision for your MUD, then this will obviously be different for you.

 ### Loot hierarchy and inheritance

At the bottom of the "loot pyramid" are global loot groups and tables. These are the basis of other, more specific loot tables to be defined later. These loot structures are so-called because they are not defined in a specific area file, but rather in the global "loot file" defined in `mud98.cfg`.

Loot groups are fairly straight forward; they provide a list of weighted drops, and define the number of times to roll on the list. They don't have inheritance; that's the job of loot tables, which aggregate and modify loot groups. Here's what a loot hierarchy might look like:

```
         LOOT GROUPS                LOOT TABLES
    +------------------+    
    |     COINS_T2     |---+
    +------------------+   |
                           |
    +------------------+   |    +------------------+
    |     COMMON_T2    |---+--->|    GLOBAL_T2     |
    +------------------+   |    +--------+---------+
                           |             |
    +------------------+   |             |
    |     RARE_T2      |---+             | (parent)
    +------------------+                 |
                                         V
    +------------------+        +------------------+
    |  ORC_CAVE_COMMON |---+--->|     ORC_CAVE     |
    +------------------+   |    +------------------+
                           |
    +------------------+   |
    |   ORC_CAVE_RARE  |---+
    +------------------+
```

In the above diagram, the global loot table `GLOBAL_T2` is comprised of loot groups `COINS_T2`, `COMMON_T2`, and `RARE_T2`, each with its own internal list of items and coins with their respective weights. Loot table `GLOBAL_T2` assigns each group a chance to roll (with common being more likely than rare; but that's up to how the loot table is built).

Below the globals, there is a prospective "Orc Cave" area. Instead of manually including the global loot elements, it specifies groups with only area-specific additions to the loot pool: `ORC_CAVE_COMMON` and `ORC_CAVE_RARE`. These are included in a new, area-specific loot table, `ORC_CAVE`.

Here is where loot table inheritance comes into play. The loot table `ORC_CAVE` has `GLOBAL_T2` as its parent. This means that the `ORC_CAVE` loot table is actually a superset of `GLOBAL_T2`, and has an inherent chance of rolling on `COINS_T2`, `COMMON_T2`, and `RARE_T2` in addition to its own groups, `ORC_CAVE_COMMON` and `ORC_CAVE_RARE`.

This is a powerful and flexible way to quickly build reusable and consistent loot tables all across your game world.

## OLC Editing

The loot system is fully editable via OLC commands. There are three ways to access loot editing:

### Global Loot Editing

Use `edit loot` to edit the global loot database (groups and tables not owned by any specific entity).

### Area Loot Editing

From within `aedit`, use the `loot` command to enter the loot sub-editor for that area. Any groups and tables you create will be owned by the area.

### Entity-Specific Editing

The loot editor can also be accessed from `medit` and `oedit` for mob-specific or object-specific loot (if your game design requires it).

## Loot Editor Commands (ledit)

When in the main loot editor, the following commands are available:

| Command | Usage | Description |
|---------|-------|-------------|
| `list` | `list` | Show all loot groups and tables for this entity |
| `show` | `show` | Display loot configuration summary |
| `edit` | `edit <group\|table>` | Edit a loot group or table (enters sub-editor) |
| `groups` | `groups` | List all loot groups (detailed) |
| `tables` | `tables` | List all loot tables (detailed) |
| `create` | `create group\|table <args>` | Create a new group or table (and enter sub-editor) |
| `delete` | `delete group\|table <name>` | Delete a group or table |
| `add` | `add item\|cp\|op <args>` | Add an item, cp, or operation (legacy inline mode) |
| `remove` | `remove entry\|op <args>` | Remove an entry or operation (legacy inline mode) |
| `parent` | `parent <table> [parent\|none]` | Set or clear a table's parent |
| `save` | `save [json\|olc]` | Save loot data (optionally force format) |
| `olist` | `olist` | List all objects in the edited loot area |
| `help` | `help [command]` | Show help for loot editor commands |
| `done` | `done` | Exit the loot editor |

### Creating Groups and Tables

When you create a group or table, you automatically enter a dedicated sub-editor for that element:

```
create group COINS_T1 1        -> Creates group with 1 roll, enters group editor
create table MOB_ORC           -> Creates table, enters table editor
create table MOB_ORC_BOSS parent MOB_ORC   -> Creates table with parent
```

### Editing Groups and Tables

Use `edit <name>` to enter the sub-editor for an existing group or table:

```
edit COINS_T1                  -> Enters group sub-editor for COINS_T1
edit MOB_ORC                   -> Enters table sub-editor for MOB_ORC
```

## Loot Group Sub-Editor

When editing a loot group, the prompt changes to show `LGrp` and the following commands are available:

| Command | Usage | Description |
|---------|-------|-------------|
| `show` | `show` | Show group details |
| `add item` | `add item <vnum> <weight> <min> <max>` | Add an item entry |
| `add cp` | `add cp <weight> <min> <max>` | Add a choice point (currency) |
| `remove` | `remove <index>` | Remove entry by 1-based index |
| `rolls` | `rolls <number>` | Set number of rolls |
| `olist` | `olist` | List available items in the area |
| `done` | `done` | Exit group editor (back to ledit) |

**Example session:**
```
> edit COINS_T1
Entering loot group editor. Type 'done' to exit, '?' for help.
Loot Group: COINS_T1
Owner:      Yes
Rolls:      1
Entries:    0

[LGrp]> add cp 100 80 160
Added cp (w=100 q=80-160).

[LGrp]> show
Loot Group: COINS_T1
Owner:      Yes
Rolls:      1
Entries:    1

Entries:
  [ 1] cp w=100 q=80-160

[LGrp]> done
Exiting loot group editor.
```

## Loot Table Sub-Editor

When editing a loot table, the prompt changes to show `LTbl` and the following commands are available:

| Command | Usage | Description |
|---------|-------|-------------|
| `show` | `show` | Show table details |
| `add op use_group` | `add op use_group <group> [chance]` | Reference a loot group |
| `add op add_item` | `add op add_item <vnum> [chance] [min] [max]` | Add item directly |
| `add op add_cp` | `add op add_cp [chance] [min] [max]` | Add cp directly |
| `add op mul_cp` | `add op mul_cp <factor>` | Multiply all cp amounts |
| `add op mul_all` | `add op mul_all <factor>` | Multiply all chances |
| `add op remove_item` | `add op remove_item <vnum>` | Remove an item |
| `add op remove_group` | `add op remove_group <group>` | Remove a group reference |
| `remove` | `remove <index>` | Remove operation by 1-based index |
| `parent` | `parent [name\|none]` | Set or clear parent table |
| `done` | `done` | Exit table editor (back to ledit) |

**Example session:**
```
> create table MOB_ORC parent GLOBAL_T1
Created loot table 'MOB_ORC' with parent 'GLOBAL_T1'.
Entering loot table editor. Type 'done' to exit, '?' for help.
Loot Table: MOB_ORC
Owner:      Yes
Parent:     GLOBAL_T1
Operations: 0

[LTbl]> add op use_group ORC_CAVE_COMMON 50
Added use_group operation.

[LTbl]> add op mul_cp 110
Added mul_cp operation.

[LTbl]> show
Loot Table: MOB_ORC
Owner:      Yes
Parent:     GLOBAL_T1
Operations: 2

Operations:
  [ 1] use_group group=ORC_CAVE_COMMON a=50 b=0 c=0 d=0
  [ 2] mul_cp group= a=110 b=0 c=0 d=0

[LTbl]> done
Exiting loot table editor.
```

## Persistence

Loot data is persisted alongside area data in both ROM-OLC (`.are`) and JSON (`.json`) formats.

### ROM-OLC Format

In `.are` files, loot is stored in a `#LOOT` / `#ENDLOOT` section:

```
#LOOT

group AREA_MATS_GLOOMCAVES 2
  item 3401 1 2  weight 70
  item 3402 1 1  weight 30

table AREA_GLOOMCAVES_T1 parent GLOBAL_T1
  use_group AREA_MATS_GLOOMCAVES 50
  mul_cp 110

#ENDLOOT
```

### JSON Format

In `.json` area files, loot is stored under a `"loot"` key containing `"groups"` and `"tables"` arrays:

```json
{
  "loot": {
    "groups": [
      {
        "name": "AREA_MATS_GLOOMCAVES",
        "rolls": 2,
        "entries": [
          { "type": "item", "vnum": 3401, "minQty": 1, "maxQty": 2, "weight": 70 },
          { "type": "item", "vnum": 3402, "minQty": 1, "maxQty": 1, "weight": 30 }
        ]
      }
    ],
    "tables": [
      {
        "name": "AREA_GLOOMCAVES_T1",
        "parent": "GLOBAL_T1",
        "ops": [
          { "op": "use_group", "group": "AREA_MATS_GLOOMCAVES", "chance": 50 },
          { "op": "mul_cp", "multiplier": 110 }
        ]
      }
    ]
  }
}
```

### Saving

The `save` command in `ledit` saves the loot data:
- `save` - Saves using the default format (based on area file extension)
- `save json` - Forces JSON format
- `save olc` - Forces ROM-OLC format

When editing area loot via `aedit > loot`, changes are automatically marked and saved with the area when you use `asave`.
