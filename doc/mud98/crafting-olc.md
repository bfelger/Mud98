# Builder's Guide to Crafting

This guide covers how to create and configure crafting content using the Online Creation (OLC) system.

---

## Overview

Builders can create:
- **Materials** (`ITEM_MAT`) — Crafting ingredients
- **Workstations** (`ITEM_WORKSTATION`) — Required locations for crafting
- **Recipes** — Instructions for creating items
- **Harvestable corpses** — Mobiles that drop materials when skinned/butchered
- **Salvageable items** — Equipment that yields materials when salvaged

---

## Item Types

### Materials (`ITEM_MAT`)

Materials are crafting ingredients used in recipes.

**Creating a Material in OEdit**:

```
oedit create 12001
name iron ore~
short an iron ore chunk~
long An iron ore chunk lies here.~
type material
v0 1        (material type: see craft_mat_type_table)
v1 1        (amount/stack size)
v2 0        (quality: 0-100)
```

**Material Types** (value[0]):
| Value | Type | Description |
|-------|------|-------------|
| 0 | none | Unspecified |
| 1 | metal | Ores, ingots, metal scraps |
| 2 | leather | Hides, pelts, leather |
| 3 | cloth | Fabric, thread, silk |
| 4 | wood | Lumber, branches, bark |
| 5 | gem | Precious stones |
| 6 | herb | Plants, flowers, roots |
| 7 | meat | Food ingredients |
| 8 | bone | Bones, teeth, horns |
| 9 | essence | Magical components |
| 10 | misc | Everything else |

**Value Fields**:
- `value[0]` — Material type (CraftMatType enum)
- `value[1]` — Amount/quantity in this stack
- `value[2]` — Quality (0-100, affects crafting results)

---

### Workstations (`ITEM_WORKSTATION`)

Workstations are objects where players craft items.

**Creating a Workstation in OEdit**:

```
oedit create 12010
name forge~
short a blacksmith's forge~
long A large forge burns with intense heat.~
type workstation
v0 2        (workstation type flags: FORGE)
wear none
extra !get !drop
```

**Workstation Types** (value[0] — bit flags, can combine):
| Bit | Type | Description |
|-----|------|-------------|
| 1 | FORGE | Metalworking |
| 2 | ANVIL | Smithing |
| 4 | TANNING_RACK | Leatherwork |
| 8 | LOOM | Weaving |
| 16 | ALCHEMY_LAB | Potion brewing |
| 32 | COOKING_FIRE | Food preparation |
| 64 | JEWELER_BENCH | Jewelry crafting |
| 128 | WORKBENCH | General crafting |
| 256 | ENCHANTING_CIRCLE | Magical work |

**Combining Types**: Add values together. A `forge + anvil` = value 3.

**Placement**: Put workstations in rooms using `oedit reset` or area resets.

---

## Recipe Editor (RecEdit)

The Recipe Editor creates and modifies crafting recipes.

### Entering RecEdit

```
recedit <vnum>      Enter editor for existing recipe
recedit create      Create new recipe (assigns next available VNUM)
```

### RecEdit Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| `show` | `show` | Display current recipe |
| `name` | `name <text>` | Set recipe name |
| `skill` | `skill <skillname>` | Set required skill |
| `minskill` | `minskill <1-100>` | Minimum skill percentage |
| `level` | `level <1-60>` | Minimum player level |
| `output` | `output <vnum>` | VNUM of crafted item |
| `workstation` | `workstation <type>` | Required workstation type |
| `wsvnum` | `wsvnum <vnum>` | Require specific workstation VNUM |
| `discovery` | `discovery <type>` | How recipe is learned |
| `addmat` | `addmat <vnum> <qty>` | Add material requirement |
| `delmat` | `delmat <index>` | Remove material by index |
| `clearmat` | `clearmat` | Remove all materials |
| `done` | `done` | Exit editor |

### Example: Creating a Recipe

```
recedit create
name leather armor~
skill leatherworking
minskill 25
level 10
output 12050
workstation tanning_rack
addmat 12001 3      (3x wolf pelt, VNUM 12001)
addmat 12002 1      (1x leather strips, VNUM 12002)
discovery known
done
```

### Discovery Types

| Type | Description |
|------|-------------|
| `known` | All players know this recipe |
| `trainer` | Learned from NPC trainers |
| `quest` | Reward from completing quests |
| `drop` | Found as loot |
| `exploration` | Discovered in the world |

---

## Area Editor Integration (AEdit)

Recipes belong to areas. Use AEdit to manage area recipes.

### AEdit Recipe Commands

```
aedit <area>
recipe list                 List all recipes in this area
recipe create               Create new recipe in this area
recipe edit <vnum>          Edit existing recipe
recipe delete <vnum>        Remove recipe from area
```

---

## Configuring Harvestable Corpses

Mobiles can drop materials when their corpses are skinned or butchered.

### In MEdit

```
medit <vnum>
craftmats add <vnum>        Add skinnable/butcherable material
craftmats remove <index>    Remove material
craftmats clear             Clear all materials
craftmats show              Display current materials
```

**Example**:
```
medit 12100
craftmats add 12001         (wolf pelt)
craftmats add 12005         (wolf meat)
```

When this mobile dies, players can:
- `skin corpse` → obtain wolf pelt
- `butcher corpse` → obtain wolf meat

### Material Assignment

The system determines skin vs. butcher based on material type:
- **Skinnable**: leather, cloth (hides, pelts, fur)
- **Butcherable**: meat, bone, misc body parts

---

## Configuring Salvageable Items

Equipment can yield materials when salvaged.

### In OEdit

```
oedit <vnum>
salvagemats add <vnum>      Add salvage material
salvagemats remove <index>  Remove material
salvagemats clear           Clear all materials
salvagemats show            Display current materials
```

**Example**:
```
oedit 12200
name rusty sword~
type weapon
salvagemats add 12001       (iron scraps)
salvagemats add 12010       (leather scraps from handle)
```

When players `salvage rusty sword`, they receive the configured materials (yield varies by skill).

---

## Object List Command (OList)

The enhanced `olist` command helps find materials:

```
olist                       List all objects in area
olist mat                   List only ITEM_MAT objects
olist mat metal             List metal materials only
olist mat leather           List leather materials only
```

---

## Testing Recipes

### Quick Test Checklist

1. **Create the output item** (the thing being crafted)
2. **Create material items** (ITEM_MAT type)
3. **Create workstation** (if required)
4. **Create recipe** linking them together
5. **Place workstation** in a room
6. **Test as player**:
   - Get materials
   - Go to workstation
   - `craft <recipe name>`

### Common Issues

| Problem | Solution |
|---------|----------|
| "You don't know that recipe" | Check discovery type, player level |
| "You lack the skill" | Player needs to learn/practice the skill |
| "You're not skilled enough" | Player skill % below recipe minimum |
| "You don't have the materials" | Check material VNUMs match |
| "You need a workstation" | Place correct workstation in room |
| Recipe not showing in `recipes` | Check discovery type is `known` or player learned it |

---

## Data File Formats

### ROM-OLC Format

Recipes are saved in area files with the `#RECIPES` section:

```
#RECIPES
#RECIPE
Vnum 12001
Name leather armor~
Skill leatherworking~
MinSkill 25
MinLevel 10
OutputVnum 12050
WorkstationType 4
Discovery known~
#INGREDIENT
MatVnum 12001
Quantity 3
#INGREDIENT
MatVnum 12002
Quantity 1
#ENDRECIPE
#ENDRECIPES
```

### JSON Format

```json
{
  "recipes": [
    {
      "vnum": 12001,
      "name": "leather armor",
      "skill": "leatherworking",
      "minSkill": 25,
      "minLevel": 10,
      "outputVnum": 12050,
      "workstationType": "tanning_rack",
      "discovery": "known",
      "ingredients": [
        { "matVnum": 12001, "quantity": 3 },
        { "matVnum": 12002, "quantity": 1 }
      ]
    }
  ]
}
```

---

## Best Practices

1. **Organize by area** — Keep related recipes, materials, and workstations in the same area
2. **Use clear names** — Recipe names should be intuitive for players
3. **Balance requirements** — Consider material availability and skill requirements
4. **Test the full loop** — Verify harvesting → crafting → using the item
5. **Document rare recipes** — Note where discovery-type recipes can be found
6. **Set appropriate levels** — Match recipe levels to where materials drop

---

## See Also

- [crafting.md](crafting.md) — Player-facing crafting guide
- [olc.hlp](../../area/olc.hlp) — General OLC help
