# Mud98 Lox Recipes

This playbook captures common scripting scenarios drawn from the [Faladrin Forest walkthrough](doc/mud98/wb-02-new-beginnings.md), automated tests, and live game usage. Each recipe lists prerequisites, step-by-step instructions, Lox snippets, and verification tips so builders (and AI tooling) can apply them quickly.

## Table of Contents
1. [Login Prologue & Onboarding Prompt](#login-prologue--onboarding-prompt)
2. [Quest Granting & Follow-Up Messaging](#quest-granting--follow-up-messaging)
3. [Quest Turn-In & Reward Delivery](#quest-turn-in--reward-delivery)
4. [Tutorial Notes & Room Guidance](#tutorial-notes--room-guidance)
5. [Delayed Announcements](#delayed-announcements)
6. [Scripted Commands & Emotes](#scripted-commands--emotes)
7. [Object Exchanges & Inventory Events](#object-exchanges--inventory-events)
8. [Object / Room / Mob Event Tips](#object--room--mob-event-tips)
9. [Faction & Reputation Hooks](#faction--reputation-hooks)
10. [Global Scans & Telemetry](#global-scans--telemetry)

Each section cross-links to the language/runtime/API guides where relevant.

---

## Login Prologue & Onboarding Prompt
**Goal**: Narrate the player’s arrival and instruct them immediately after login.

**Prereqs**
- Area room with a reserved VNUM (e.g., `12000`).
- You have OLC access (`redit`, `event`).

**Steps**
1. `redit <vnum>` → `lox`.
2. Paste or author `on_login(vch)`:
   ```lox
   on_login(vch) {
       if (vch.level > 1 or vch.race != Race.Elf)
           return;

       vch.send(
           "^*You step off the end of an old rope ladder...^/^/"
           "Now you must go out into the world and find your destiny...^/^x")
   }
   ```
3. Validate with `.v`; fix reported line numbers if compilation fails.
4. `@` to assign; Mud98 reports `Class "room_<vnum>" ... assigned`.
5. `event set login` (accept defaults unless you need criteria).
6. Persist (`asave changed` or `asave area`).

**Verification**
- Create a disposable level-1 character matching your race gate.
- Log in; the scripted text should appear before the room description (`doc/mud98/wb-02-new-beginnings.md:579`).
- If nothing fires, run `event list` within `redit` to ensure the login trigger exists.

---

## Quest Granting & Follow-Up Messaging
**Goal**: Start a quest after onboarding and reinforce actions via delayed tips.

**Steps**
1. Within `on_login`, add:
   ```lox
   if (vch.can_quest(12000)) {
       delay(1, () -> {
           vch.send("^jYou have instructions...^x")
           vch.grant_quest(12000)
           vch.send("^jType '^*QUEST^j' to view your quest log.^x")
       })
   }
   ```
2. Confirm the quest exists in OLC (`qedit`) or JSON (`quests[]`) and that the VNUM matches.
3. Optional: use multiple `delay()` calls to stagger narrative beats or play sounds (`do("emote ...")`).

**Verification**
- Run through character creation; after login you should receive the quest message followed by a quest log entry (`doc/mud98/wb-02-new-beginnings.md:1046`).
- Use the `QUEST` command and confirm the entry appears only once (subsequent logins should hit the `can_quest` guard and skip the grant).

---

## Quest Turn-In & Reward Delivery
**Goal**: Finish a quest automatically when the player greets an NPC.

**Steps**
1. `medit <quest npc>` → `lox`.
2. Script:
   ```lox
   on_greet(vch) {
       if (vch.can_finish_quest(12000)) {
           say("There you are, ${vch.name}; I'm glad to see you...")
           echo("$n sighs.")
           say("I need your help with something, and it's not going to be pleasant.")
           vch.finish_quest(12000)
       }
       else if (vch.has_quest(12000)) {
           say(vch, "Stay focused, ${vch.name}. The forest still needs you.")
       }
   }
   ```
3. `event set greet`, `asave changed`.

**Verification**
- Complete the quest objectives, enter the NPC’s room, and wait for the `greet` trigger.
- You should see completion text, XP/currency rewards, and any faction adjustments (`doc/mud98/wb-02-new-beginnings.md:1122`).
- Test the `else if` branch by greeting mid-quest; it should provide reminder dialogue.

---

## Tutorial Notes & Room Guidance
**Goal**: Provide contextual hints when players enter training rooms.

**Steps**
1. Add scenery via `ed add note` or `exdesc` fields.
2. Script:
   ```lox
   on_greet(vch) {
       if (vch.has_quest(12000)) {
           vch.send("^jTo read the note, try ^*READ NOTE^j, ^*LOOK NOTE^j, or ^*EXAMINE NOTE^j.^x")
       }
   }
   ```
3. `event set greet`, `asave`.

**Variations**
- Swap `has_quest` for faction, class, or alignment checks.
- Use `delay()` inside the greet event to present multiple hints without flooding the player buffer.

**Verification**: Enter the room with an eligible quest active; the hint should fire once per entry (`doc/mud98/wb-02-new-beginnings.md:1447`).

---

## Delayed Announcements
**Goal**: Send follow-up information after the room description or other events.

**Pattern**
```lox
delay(1, () -> {
    vch.send("^jYou have instructions...^x")
})
```

- Delay units are area pulses (default 8 seconds). Keep them short for onboarding, longer for dramatic pauses.
- You can nest delays or capture data (quests, timers) inside the closure—they remain alive until execution (`Language Guide` → Closures).
- Combine with `if (!vch.has_quest(...)) return;` to avoid re-sending to disqualified players.

---

## Scripted Commands & Emotes
**Goal**: Invoke standard commands (`say`, `open`, `smile`, etc.) from scripts using the Mud98 interpreter.

**Steps**
1. Call `do("command args")` within any callback where `exec_context.me` is meaningful (player or mob events).
2. Examples:
   ```lox
   do("say Welcome, ${vch.name}!")
   do("emote bows low to $N")
   do("open portal")
   ```

**Tips**
- Interpolate strings to personalize output, but escape quotes carefully.
- Track state on the entity (`this.hasGreeted`) to prevent repeated commands.
- For advanced use (invoking specific DoFuncs), fetch entries from `native_cmds` but default to `do()` for safety.

---

## Object Exchanges & Inventory Events
**Goal**: React when players give, take, or drop objects.

**Object `on_given` example**
```lox
on_given(giver, taker) {
        if (!taker.is_mob())
        return;

    taker.say(giver, "Thank you for the offer.")
    giver.adjust_reputation(Faction.Tauremar, 5)
}
```

**Room `on_dropped` example**
```lox
on_dropped(obj, dropper) {
    if (obj.vnum == 12050) {
        dropper.send("^jThe soil rejects the seed and blows it back at you.^x")
        do("get seed")
    }
}
```

**Setup**
- Objects: `oedit <vnum>` → `lox`, define `on_given`, `on_taken`, or `on_dropped`; `event set <trigger>`.
- Rooms/mobs: use `on_give`, `on_drop`, etc., depending on context.
- Criteria: `event set given keyword seed` restricts the trigger to specific items.

---

## Object / Room / Mob Event Tips
- Rooms/mobs share `greet`, `entry`, `random`, `fight`, etc.; objects use `given`, `taken`, `dropped`.
- Signature reminder: `on_give(ch, obj)` for mobs vs. `on_given(giver, taker)` for objects. See [Runtime Guide](runtime.md#events--triggers) for the full table.
- You can attach multiple events to a single method. For clarity, branch on `this.header.event_triggers` or add helper methods per trigger.
- Remember that `greet` fires for every entrant (player and NPC). Filter with `if (vch.pcdata == NULL) return;` to suppress NPC chatter.

---

## Faction & Reputation Hooks
**Example**
```lox
on_greet(vch) {
    if (vch.is_enemy(Faction.Tauremar)) {
        say(vch, "Begone, traitor!")
        return;
    }

    vch.adjust_reputation(Faction.Tauremar, 5)
    say(vch, "Stay the course, friend of the forest.")
}
```

- Combine with quest completion (`finish_quest`) or favors (give/take events) to reinforce role-play.
- Use `get_reputation` to branch dialogue by standing tiers (e.g., Friendly vs. Hostile).

---

## Global Scans & Telemetry
**Goal**: Iterate over all areas/rooms/mobs/objects to collect stats or apply maintenance.

**Examples**
```lox
fun count_unscripted_mobs() {
    var count = 0
    global_mobs.each((mob) -> {
        if (mob.script == nil)
            count++
    })
    return count
}
```

```lox
global_objs.each((index, obj) -> {
    if (obj.area.vnum >= 12000 && obj.area.vnum <= 12099 && obj.is_obj())
        obj.send("A gentle breeze rustles nearby.")
})
```

**Notes**
- Arrays/tables/lists pass different arguments to `.each` (see [Language Guide](language.md#data-structures)).
- These scans run synchronously; avoid heavy work every pulse. Consider gating behind imm commands or scheduled events.

---

Have a scenario you reuse often? Extend this playbook and link back to the [Language](language.md), [Runtime](runtime.md), or [API](api.md) sections so future builders.
