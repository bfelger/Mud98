# Player's Guide to Crafting

This guide covers the crafting system available to players in Mud98.

---

## Overview

The crafting system allows players to:
- **Harvest** materials from creature corpses (skinning, butchering)
- **Salvage** materials from equipment
- **Craft** new items using recipes and materials

Crafting uses skills that improve with practice. Higher skill means better success rates and higher quality results.

---

## Crafting Skills

The following skills are available:

| Skill | Description |
|-------|-------------|
| `skinning` | Harvest hides and pelts from corpses |
| `butchering` | Harvest meat and body parts from corpses |
| `leatherworking` | Craft leather armor and goods |
| `blacksmithing` | Forge metal weapons and armor |
| `tailoring` | Create cloth items and clothing |
| `alchemy` | Brew potions and elixirs |
| `cooking` | Prepare food items |
| `jewelcraft` | Create rings, amulets, and jewelry |
| `woodworking` | Craft wooden items (bows, staves, furniture) |
| `enchanting` | Add magical properties to items |

Skills can be learned during character creation or from trainers. Use `practice <skill>` to improve them.

---

## Harvesting Materials

### Skinning

The `skin` command harvests hides and pelts from creature corpses.

**Syntax**: `skin <corpse>`

**Requirements**:
- The corpse must have skinnable materials defined
- The corpse must not already be skinned
- You need the `skinning` skill

**Example**:
```
> skin corpse
You carefully skin the corpse of a wolf, obtaining a wolf pelt.
```

A failed skill check means you botch the attempt and get nothing.

---

### Butchering

The `butcher` command harvests meat and body parts from creature corpses.

**Syntax**: `butcher <corpse>`

**Requirements**:
- The corpse must have butcherable materials defined
- The corpse must not already be butchered
- You need the `butchering` skill

**Example**:
```
> butcher corpse
You butcher the corpse of a deer, obtaining some venison.
```

---

### Salvaging

The `salvage` command breaks down equipment into raw materials.

**Syntax**: `salvage <item>`

**Requirements**:
- The item must have salvageable materials defined
- The item is destroyed in the process

**Skill Used** (based on item type):
- Weapons → `blacksmithing`
- Metal armor → `blacksmithing`
- Leather armor → `leatherworking`
- Cloth armor → `tailoring`
- Jewelry → `jewelcraft`
- Other items → no skill required

**Yield**:
- No skill required: 25% of materials
- Skill check failed: 50% of materials
- Skill check passed: 75-100% of materials

**Example**:
```
> salvage sword
You salvage a rusty sword, obtaining some iron scraps.
```

---

## Crafting Items

### Viewing Recipes

The `recipes` command shows available recipes.

**Syntax**: 
- `recipes` — List all known recipes
- `recipes <name>` — Show details for a specific recipe

**Example**:
```
> recipes
Known Recipes:
  leather armor       (leatherworking 25%)  Level 10
  iron sword          (blacksmithing 30%)   Level 15
  healing potion      (alchemy 20%)         Level 8

> recipes leather armor
Recipe: leather armor
Skill: leatherworking (25% required)
Level: 10
Ingredients:
  - 3x wolf pelt
  - 1x leather strips
Workstation: tanning rack
```

---

### Crafting

The `craft` command creates items from recipes.

**Syntax**: `craft <recipe name>`

**Requirements**:
- You must know the recipe (or it's commonly known)
- You must have the required skill at the minimum percentage
- You must have all required materials in your inventory
- You must be at the required workstation (if any)
- You must meet the minimum level requirement

**Example**:
```
> craft leather armor
You begin crafting leather armor...
You successfully craft a leather armor!
```

---

## Quality Tiers

Successful crafts produce items of varying quality based on your skill check:

| Quality | Description |
|---------|-------------|
| Poor | Below-average item, reduced stats |
| Normal | Standard item as designed |
| Fine | Above-average, slightly improved |
| Exceptional | Significantly improved stats |
| Masterwork | Best possible quality, rare |

Higher skill percentages and character level improve your chances of crafting higher-quality items.

---

## Workstations

Some recipes require specific workstations. Common workstation types:

| Type | Used For |
|------|----------|
| Forge | Metal weapons and armor |
| Anvil | Smithing and metalwork |
| Tanning Rack | Leather goods |
| Loom | Cloth items |
| Alchemy Lab | Potions and elixirs |
| Cooking Fire | Food preparation |
| Jeweler's Bench | Jewelry |
| Workbench | General crafting |
| Enchanting Circle | Magical enhancement |

Workstations are found throughout the world. Some may be in towns, others in dungeons or hidden locations.

---

## Tips

1. **Practice your skills** — Higher skill means better success rates and quality
2. **Gather materials** — Kill creatures and salvage old equipment
3. **Find workstations** — Explore to find the workstations you need
4. **Check recipes** — Use `recipes <name>` to see exact requirements
5. **Level up** — Some recipes have level requirements
6. **Experiment** — Some recipes must be discovered through exploration or quests

---

## See Also

- `help skills` — View your current skill levels
- `help groups` — View skill groups including crafting
- `help practice` — How to improve skills
