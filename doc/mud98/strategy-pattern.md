# Use of the Strategy Pattern in Mud98

This document details how the Strategy Pattern is used to implement Seams in Mud98, and why.

## Working with legacy code

A lot of architectural decisions in Mud98 are influenced by Michael C. Feather's book, "Working Effectively with Legacy Code". He advocates a Test-Driven Development (TDD) approach to preserve existing functionality and to prevent inadvertent breakages. But how can you test a large, tightly-coupled code-base like a ROM 2.4 derivative?

### Seams

To fully test our legacy code (as well as perform A/B tests between old and new implementations), Feathers suggests implementing a couple of concepts into the legacy code, first:

> #### Seam
> A seam is a place where you can alter behavior in your program without editing in that place..

> #### Enabling Point
> Every seam has an enabling point, a place where you can make the decision to use one behavior or another.

In other words, we need to establish clear boundaries between modules that may need to independently mutate, and we need to test these boundaries to the n<sup>th</sup> degree.

Since we need to be able to be able to swap functionality (as well as mock it for testing), the Strategy pattern is the best strategy (pun intended).

### Strategy Pattern

In the popular book, "Design Patterns: Elements of Reusable Object-Oriented Software" by Gamma, Helm, Johnson, and Vlissides, the Strategy pattern is intended to:

> Define a family of algorithms, encapsulate each one, and make them interchangeable. Strategy lets the algorithm vary independently from clients that use it.

Now, Mud98, being written in C, isn't Object-Oriented; but the ostensible goals of OOP are very attractive for the task of creating seams and de-coupling the legacy ROM code:
- Data hiding
- Encapsulation
- Polymorphism

The Strategy pattern facilitates via requirement: if we don't achieve these goals, we can't effectively create seams. Thankfully, these mechanisms are simply giving a name to something C programmers have been doing for decades via virtual dispatch.

> [!NOTE]
> OLC, commands, skills, and magic already use a form of virtual dispatch called **virtual table dispatch**, or **vtables**.

### Virtual Dispatch

Here's what our virtual dispatch implementation of the Strategy pattern looks like:

```c
// Virtual dispatch container
typedef struct ops {
    void (*do_this)(Thing* thing);
    bool (*check_that)(Thing* other_thing);
} Ops;

// Implementation A
void do_this__impl_a(Thing* thing);
bool check_that__impl_a(Thing* other_thing);
const Ops ops__impl_a = { 
    .do_this = do_this__impl_a, 
    .check_that = check_that__impl_a 
};

// Implementation B
void do_this__impl_b(Thing* thing);
bool check_that__impl_b(Thing* other_thing);
const Ops ops__impl_b = { 
    .do_this = do_this__impl_b, 
    .check_that = check_that__impl_b 
};

// Strategy selection context

const Ops* ops_strategy = &ops__impl_a; // Set the current strategy

// Client-facing functions that have no clue which strategy is selected

void do_this(Thing* thing) {
    ops_strategy->do_this(thing);
}

bool check_that(Thing* other_thing) {
    return ops_strategy->check_that(other_thing);
}
```

## Mud98 Implementation

The biggest example of runtime Strategy swapping is the persistence layer. To create the JSON persistence layer, we first had to define a seam and put the legacy implementation (ROM-OLC) behind it. Areas, for example:

```c
// src/persist/area/area_persist.h
typedef struct area_persist_format_t {
    const char* name; // e.g., "rom-olc", "json".
    PersistResult (*load)(const AreaPersistLoadParams* params);
    PersistResult (*save)(const AreaPersistSaveParams* params);
} AreaPersistFormat;

// src/persist/area/rom-olc/area_persist_rom_olc.c
const AreaPersistFormat AREA_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = persist_area_rom_olc_load,
    .save = persist_area_rom_olc_save,
};

// src/persist/area/json/area_persist_json.c
const AreaPersistFormat AREA_PERSIST_JSON = {
    .name = "json",
    .load = json_load,
    .save = json_save,
};
```

Nothing in the code needs to know _how_ these functions save and load `Area` files. Therefore, we can hide all the concrete details as `static` functions in their respective implementations, away from the rest of the code base.

Here are a few more uses of the Strategy pattern to define seams:

### CombatOps Seam

```c
// src/combat/combat_ops.h
typedef struct combat_ops_t {
    int16_t (*apply_damage)(Mobile* victim, int16_t dam, DamageType dt, bool show);
    int16_t (*apply_healing)(Mobile* victim, int16_t amount, bool show);
    int16_t (*apply_regen)(Mobile* victim, int16_t amount, RegenType regen_type);
    int16_t (*drain_mana)(Mobile* ch, int16_t amount);
    int16_t (*drain_move)(Mobile* ch, int16_t amount);
    bool (*check_hit)(Mobile* ch, Mobile* victim, DamageType dt);
    int16_t (*calculate_damage)(Mobile* ch, Mobile* victim, Object* wield, SKNUM sn, DamageType dt);
    void (*handle_death)(Mobile* ch, Mobile* victim);
} CombatOps;

extern const CombatOps* combat; // Global strategy pointer

// src/combat/combat_rom.c - ROM implementation
const CombatOps combat_rom = {
    .apply_damage = rom_apply_damage,
    .apply_healing = rom_apply_healing,
    .apply_regen = rom_apply_regen,
    .drain_mana = rom_drain_mana,
    .drain_move = rom_drain_move,
    .check_hit = rom_check_hit,
    .calculate_damage = rom_calculate_damage,
    .handle_death = rom_handle_death,
};

// src/tests/mock_combat.c - Mock implementation for testing
const CombatOps mock_combat_ops = {
    .apply_damage = mock_apply_damage,
    .apply_healing = mock_apply_healing,
    .apply_regen = mock_apply_regen,
    .drain_mana = mock_drain_mana,
    .drain_move = mock_drain_move,
    .check_hit = mock_check_hit,
    .calculate_damage = mock_calculate_damage,
    .handle_death = mock_handle_death,
};
```

The combat seam allows tests to control hit probability, damage amounts, and death handling. This makes combat tests deterministic rather than relying on RNG. We make use of this to validate (as just one example) events like `TRIG_DEATH`, which are normally heavily affects by RNG.

### SkillOps Seam

```c
// src/skills/skill_ops.h
typedef struct skill_ops_t {
    bool (*check)(Mobile* ch, SKNUM sn);
    bool (*check_simple)(Mobile* ch, SKNUM sn);
    bool (*check_modified)(Mobile* ch, SKNUM sn, int modifier);
} SkillOps;

extern const SkillOps* skill_ops; // Global strategy pointer

// src/skills/skill_ops_rom.c - ROM implementation
const SkillOps skill_ops_rom = {
    .check = rom_skill_check,
    .check_simple = rom_skill_check_simple,
    .check_modified = rom_skill_check_modified,
};

// src/tests/mock_skill_ops.c - Mock implementation
const SkillOps mock_skill_ops = {
    .check = mock_skill_check,
    .check_simple = mock_skill_check_simple,
    .check_modified = mock_skill_check_modified,
};
```

The skill seam enables tests to control skill check outcomes, making ability-dependent commands (backstab, hide, steal, etc.) predictable and testable.

## Benefits

Using the Strategy pattern for seams provides:

1. **Testability**: Mock implementations allow deterministic, isolated unit tests.
2. **Maintainability**: Clear boundaries between subsystems reduce coupling.
3. **Flexibility**: Easy to swap implementations at runtime.
4. **Safety**: Can A/B test new implementations against legacy without modifying original code.
5. **Documentation**: Interface definitions serve as contracts documenting expected behavior.
