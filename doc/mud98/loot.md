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
