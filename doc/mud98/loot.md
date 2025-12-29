# Loot Drops

Loot is rolled on tables composed of inheritable loot groups.

## Loot Group

A reusable weighted list of entries. Each entry is either:
- An item VNUM with a quantity range, or
- A currency range (in copper pieces)

## Loot Table

Loot table is an aggregation of Loot Groups, with tailored alterations:
- Add Items
- Remove items
- Add Currency
- Currency multipliers
- Chance multipliers

Tables are inheritable, so you can have per-area loot tables that build off of global loot tables.

## How it's used

Here's a rough overview of what some `loot.olc` might look like:
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
