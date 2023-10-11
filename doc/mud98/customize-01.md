# Worldcrafting from Scratch Pt. 1 &mdash; New Beginnings

Mud98 isn't a MUD; it's a MUD _codebase_. The task of crafting a MUD around it is up to you. The original area files are provided if you want to experience ROM at is was back in 1998; but they aren't very serious or cohesive, and there's a good chance you don't actually want the 1950's diner in Midgaard in your fantasy-themed setting.

This document aims to create a step-by-step how-to to take a brand-new, vanilla Mud98, and build a brand-new MUD around it that is 100% yours.

## Create an implementor

You need to have a max-level character to begin customizing Mud98.

1. Boot up Mud98.
2. Log in as a new character, and complete character generation. The choices you make don't matter, as you will be able to change any aspect of your own character after leveling.
3. After finishing character creation and getting dumped in the MUD school, `QUIT` the session.
4. Open your player file (in the `player` folder) in a text editor.
5. Set your level to 60 and your security to max by finding these lines:
    ```
    Levl 3
    Sec  0
    ```
    ...and changing them to this:
    ```
    Levl 60
    Sec  9
    ```
6. Log back into the game as the new character. Now this character is an Implementor with the highest online editing security setting.

## Create a new starting area

Let's start with a brand new area. This can be the seed of a brand new network
of areas that will define _your_ MUD. For the purposes of this example, we'll make this a new starting area for one of the races.

> Why? Because while MUDs began as heavily tilted toward hack n' slash, most players want some measure of narrative immersion that isn't possible in stock ROM. So, instead of dropping new players into "MUD School", it's better to drop them into an area that gradually introduces them to the world (and the game mechanics).

Before we create this new area, we need to find a block of VNUMs that are free to use. 

### Choosing a VNUM block

Enter this command:

```c
alist orderby vnum
```

You should see a list that looks like this, showing VNUMs in order, and displayer any gaps inbetween:

```
[Num] [Area Name                  ] (lvnum-uvnum) [Filename  ] Sec [Builders  ]
[ 26] Limbo                         (1    -   30) limbo.are    [9] [None      ]
[ 43] Smurfville                    (100  -  199) smurf.are    [9] [None      ]
[  4] Plains                        (300  -  399) plains.are   [9] [None      ]
[  1] New Ofcol                     (600  -  699) ofcol2.are   [9] [None      ]
[ 37] Olympus                       (900  -  999) olympus.are  [9] [None      ]
[  6] In the Air                    (1000 - 1099) air.are      [9] [None      ]
[  2] Shire                         (1100 - 1199) shire.are    [9] [None      ]
[  0] Valhalla                      (1200 - 1299) immort.are   [9] [None      ]
[ 24] High Tower                    (1300 - 1499) hitower.are  [9] [None      ]
[ 21] Gnome Village                 (1500 - 1599) gnome.are    [9] [None      ]
...
```

You can now pick a block of VNUMs that are not assigned to any existing area. Since your goal is to create a brand new collection of areas to define your MUD, you don't want to intermingle your VNUM block with the existing ones, if possible.

One solution is to simply delete the areas you don't need. However there is a short list of areas that, if deleted, will prevent you from booting. 

The easiest solution is to start completely outside the range of the existing areas. the highest VNUM in use in ROM is 10,599 in `tohell.are`. Even in old-school ROM, which used `short int` for VNUMs, this left 22,169 remaining possible VNUMS. In Mud98, however, VNUM is `int32_t`, which leaves us... well... still well over 2 billion VNUMs. 

But don't go that high; it will mess with the formatting.

For purposes of this example, I'll choose VNUM `12000`. It's a nice, round number. I now also need an upper bound. Most existing areas have a range of 100 to 250 VNUMs. I think 100 is a good start. We can always expand it later (if we don't "VNUM stack" like some of the stock ROM areas do).

## Creating a new area

Before creating your new area, type `HELP AEDIT` and get a rough idea of the information you will need. To get started, simply type this:

```
aedit create
```

...which will bring up this blank slate of information:

```
Name:           [48] New area
File:           area48.are
Vnums:          [0-0]
Levels:         [1-60]
Sector:         [inside]
Reset:          [5] x 8 minutes; currently at 0
Always Reset:   [NO]
Age:            [0]
Security:       [9]
Builders:       [Halivar]
Credits :       [Halivar]
Flags:          [added]
```

You're now in "edit mode" with OLC. All of `AEDIT`'s extended commands can be used from the prompt (including `COMMANDS`, which lists those extended commands).

For instance, to display the above area information, you can simply type:

```
show
```

Although `SHOW` is special; it can also be displayed by hitting `Enter` on a blank prompt. There are numorous such shorthands in OLC.

Now, for the purposes of this example, I've decided to make a starting area just for elves. Here are the commands I will use to name the area, the file, and set its properties:

```
name Faladrin Forest
file faladri
sector forest
vnums 12000 121000
levels 1 5
reset 1
alwaysreset yes
```

These settings are novel to Mud98:
- `sector` sets a "default" sector value for the rooms in the area.
- `reset` is the number of "area pulses" to use before the room resets. By setting it to `1`, we guarantee this starting area (including mobs and objs) resets ever 8 minutes.
- `alwaysreset` overrides the default behavior to never reset certain things (such as container contents) if a player is in the area.

Now my `SHOW` looks like this:

```
Name:           [48] Faladrin Forest
File:           faladri.are
Vnums:          [12000-12100]
Levels:         [1-5]
Sector:         [forest]
Reset:          [1] x 8 minutes; currently at 0
Always Reset:   [YES]
Security:       [9]
Builders:       [Halivar]
Credits :       [Halivar]
Flags:          [changed added]
```

`CREDITS` can be whatever I want (but should be my Imp name, or else it will mess with the formatting on the universal, non-OLC `AREAS` command) , but `SECURITY` and `BUILDERS` is special. When I created the area, they were set (by default) to my security level and player name, respectively. There are "permissions" for the area. To edit this area, my name must be in the `BUILDERS` list (as it can have more than one name) _or_ my security level must be higher than that of the area. Thus, an Imm who has higer security than you can edit your areas; but not lower unless they are listed as a builder in that area. As an Imp with max security, I can edit _any_ area.

Now it's time to set this area is stone by saving it:

```
done
asave changed
```

We get this message:

```
Saved zones:
         Faladrin Forest - 'faladri.are'
```

If do a `git status` in my `area` folder, I see my changed files:

```
Changes not staged for commit:
        modified:   area/area.lst

Untracked files:
        area/faladri.are
```

That's it! I now have my new starting area, and I'm read to start adding to rooms to it.

> You _are_ using `git` to keep track of your changes, right?
> 
> Right?

### Creating the starting room

The command for editing rooms is `REDIT`, and it always helps to type `HELP REDIT` before you use it. Creating a room is almost as simple as creating an area; all we need is the starting VNUM:

```
redit create 12000
```

...which gives us this:

```
Room created.
Description:
Name:       []
Area:       [   48] Faladrin Forest
Vnum:       [12000]
Sector:     [forest]
Room flags: [none]
Heal rec:   [100]
Mana rec:   [100]
Characters: [halivar]
Objects:    [none]
Exits:
```

Be sure to check `COMMANDS` to see what options are available.

The intention is for this to be the first room a player sees when they create an elf character. Therefore, we should think carefully about the naming and prose used in this room. For now, I will use placeholder text.

> To be honest, I was never a good builder. In my Imp days of ages past, I was a _coding_ Imp. I needed an army of builders and admins to do the other work for me (like, actually _running_ and _creating_ the MUD).

Note that, by creating the room, we actually teleported to it. This is useful so we can perform actions like `LOOK` and whatnot. OLC is very much "in-game".

I choose to give it a narrative title, rather than a descriptive one:

```
name The Awakening
```

Even though this is a placeholder, I want to at least put _something_ for the description, using `DESC`:

```                                    
desc
-========- Entering EDIT Mode -=========-
    Type .h on a new line for help
 Terminate with a @ on a blank line.
-=======================================-
> 
```

Typing `.h` for help yields a list of really helpul commands. In particular, `.f` is your friend.

I will enter some nice, overwrought, pretentious prose:

```
You stand next to the great old trunk of Cuivealda. The light is dim, as
the dense, high canopy that forms Tauremar covers the sky. There is little
ground cover, and the forest floor is a desolate place of perpetual
twilight.
```

Use `@` on a bare line to finish the description.

The last thing to do before making it a valid "starting" area is setting room flags. Type `FLAGS` to see a list of available options.

The two flags I want:

```
flags newbies_only no_recall
```

The latter  is because I don't want people just recalling out of the start zone. There should be a narrative flow to the game, and skipping the starting area breaks that.

Now I want to hook this room up as the starting area for elves.

### Setting racial starting location

The command for editing races is `RAEDIT`. Unfortunately, there's no help for that command, yet (it wasn't a part of the orginal OLC). I'll put that on my bucket list.

For now, let's just dive in:

```
raedit elf
```

This gives us this:

```
Name        : [elf]
PC race?    : [YES]
Act         : [none]
Aff         : [infrared]
Off         : [none]
Imm         : [none]
Res         : [charm]
Vuln        : [iron]
Form        : [edible sentient biped mammal]
Parts       : [head arms legs heart brains guts hands feet fingers ear eye]
Points      : [5]
Size        : [small]
Start Loc   : [0]   (not used)
    Class      XPmult  XP/lvl(pts)   Start Loc
    Mage        100     1000( 40)    [    0]   (not used)
    Cleric      125     1000( 40)    [    0]   (not used)
    Thief       100     1000( 40)    [    0]   (not used)
    Warrior     120     1000( 40)    [    0]   (not used)

Str:12(16) Int:14(20) Wis:13(18) Dex:15(21) Con:11(15) 
 0. sneak
 1. hide
```

Notice under `Start Loc`, it says "(not used)". That means Mud98 isn't configured to use racial start locations. By default, all non-stock-ROM behavior is disabled to provide a more authentic, legacy ROM experience out-of-the-box.

Open up `mud98.cfg` in the root directory and change this setting (uncommenting it, if needed):

```
start_loc_by_race = yes
```

Note that this is a completely safe flag; it has no effect on races that don't have a start location set, yet.

> You may have noticed that there is also a class `start_loc` and a per-race-class-combo `start_loc`. These can be used with each other freely. You can have a single spawn point for mages, a single spawn point for elves, and separate spawn points for all the other race/class combos. It's meant to give you maximum flexibility in world-building. To take advantage of this, you need to also enable `start_loc_by_class` in `mud98.cfg`.

Restart Mud98 and log back in with your Imp character. Then `raedit elf` again. Note that now `Start Loc` is `0`, but the warning that it won't be used is gone.

I can now set the start location for elves:

```
start_loc 12000
```

Now `SHOW` has our new room as `Start Loc`:

```
Name        : [elf]
PC race?    : [YES]
Act         : [none]
Aff         : [infrared]
Off         : [none]
Imm         : [none]
Res         : [charm]
Vuln        : [iron]
Form        : [edible sentient biped mammal]
Parts       : [head arms legs heart brains guts hands feet fingers ear eye]
Points      : [5]
Size        : [small]
Start Loc   : [12000] The Awakening 
    Class      XPmult  XP/lvl(pts)   Start Loc
    Mage        100     1000( 40)    [    0]  (not used)
    Cleric      125     1000( 40)    [    0]  (not used)
    Thief       100     1000( 40)    [    0]  (not used)
    Warrior     120     1000( 40)    [    0]  (not used)

Str:12(16) Int:14(20) Wis:13(18) Dex:15(21) Con:11(15) 
 0. sneak
 1. hide
```

That looks correct. I commit my changes to disk:

```
done
asave changed
```

And now I'm ready to test with a new character:

```
newelfguy
y
password
password
elf
m
warrior
g
n
sword
```

Where do we end up?

```
The Awakening
You stand next to the great old trunk of Cuivealda. The light is dim, as
the dense, high canopy that forms Tauremar covers the sky. There is little
ground cover, and the forest floor is a desolate place of perpetual
twilight.
Halivar the Keeper of Knowledge is here.
You have no unread notes.
```

Oops. I forgot to log out of my Imp. But now we have a solid start on building out a newbie zone for elves.

### Creating a greeting for new characters

I want to display a banner for new characters when they enter the game, but I only want it to display once. Rather than trying to create a new field, I'm going to add a trigger to do it. Right now, the only way to add triggers is on mobs, so first I'll need to make a triggerbot.

#### Making the triggerbot

The command to create mobiles is `MEDIT`. I will create my triggerbot with this command:

```
medit create 12000
```

> Note the re-use of the VNUM. VNUM's are only unique to a certain type of entity (`RoomData`, `MobPrototype`, `ObjPrototype`, etc), and can be shared _across_ them. To have keep things less confusing, for any given VNUM `X`, I try to keep them all in one place: the `ObjPrototype` with VNUM `X` will be held by the `MobPrototype` with VNUM `X`, in the `RoomData` with VNUM `X`. Consider spacing out room VNUMs to accomodate.

Anyway, the command we entered creates a new `MobPrototype` (_not_ a `CharData`... not yet) and displays it as a blank slate:

```

Name:        [no name]
Vnum:        [ 12000]
Area:        [    48] Faladrin Forest
Level:       [     0] Sex:     [  none] Group:   [    0]
Align:       [     0] Hitroll: [     0] Dam type: [none]
Hit dice:    [  0d0  +   0] Damage dice:  [  0d0  +   0]
Mana dice:   [  0d0  +   0] Material:     [     unknown]
Race:        [       human] Size:         [      medium]
Start pos.:  [    standing] Default pos.: [    standing]
Wealth:      [    0]
Armor:       [pierce: 0  bash: 0  slash: 0  magic: 0]
Affected by: [none]
Act:         [npc]
Form:        [none]
Parts:       [none]
Imm:         [none]
Res:         [none]
Vuln:        [none]
Off:         [none]
Short descr: (no short description)
Long descr:
(no long description)
Description:
```

Because this is a triggerbot, I don't actually want much on this. I'll give it a name, make it `sentinal` so it doesn't wander, and make it invisible to players:

```
name triggerbot_12000
act sentinel
aff invis
```

That's it; that's all I care about. I'll save the bot with `done` and get cracking on making its Mob Prog.

#### Creating the Mob Prog

The command to create Mob Progs is `MPEDIT`:

```
mpedit create 12000
```

Now we are in the context of editting the Mob Prog:

```
Vnum:       [12000]
Code:
```

The editor is a line editor, much like room descriptions, using the `code` command. I will use this script:

```
mob echoat $n {*You step off the end of an old rope ladder at the base of Cuivealda, the{/Tree of Awakening. Your years in the nurturing hands of the Tetyayath have{/come to an end.{/{/Now you must go out into the world and find your destiny...{/{x
```

The `mob echoat $n` command says that the mob executing the script will perform an "echo" (unquoted text) directly to the character that prompted the trigger. I added coloration to add some flair as a call-out. The `{/` color code is actually a new-line. When doing mobprogs like this, you will need to keep track of your own line endings.

I now close off the code with `@`, and finish the MobProg with `done`.

Now I have the Mob Prog that I want `triggerbot_12000` to perform, but now I need to add the trigger.

#### Making the triggerbot trigger

I open up my triggerbot for editing again:

```
medit 12000
```

If I check `COMMANDS`, I see there is an `ADDPROG` command. A blind call yields this:

```
Syntax:   addprog [vnum] [trigger] [phrase]
```

Here's the command to add the trigger:

```
addprog 12000 act has entered the game
```

The first parameter is the VNUM of the Mob Prog (not the mob itself; we're already editing that), followed by the trigger, `act`. This means anything it can _see_. The rest of the command is an unquoted string (in this case, "has entered the game") that the triggerbot will look for in all "acts" (broadcasts to the room).

As it just so happens, the first thing anyone sees of a new character in the start location is:

```
Bob has entered the game.
```

This trigger will activate _before_ the target receives the room description. This is very handy.

Now we need to finish off `MEDIT` with `done`, and actually place the triggerbot in the room.

#### Adding triggerbot to the room

Edit the room again:

```
redit 12000
```

The command to load a mob into the room is `MRESET`.

A blind call yields the syntax:

```
Syntax:  mreset <vnum> <max #x> <min #x>
```

I only ever want one, and always one, so I enter this:

```
mreset 12000 1 1
```

Ok, so now it's added, which we verify with the `RESETS` command:

```
Resets: M = mobile, R = room, O = object, P = pet, S = shopkeeper
 No.  Loads    Description       Location         Vnum   Ar Rm Description
==== ======== ============= =================== ======== ===== ===========
[ 1] M[12000] (no short des                     R[12000]  1- 1 The Awakening  
```

It's in the room, but you won't see it, as it was marked invisible.

#### Testing the triggerbot

Now all we need to do is log in with a new character and see if the new intro text works:

```
Welcome to Mud98. Please do not feed the mobiles.

You step off the end of an old rope ladder at the base of Cuivealda, the
Tree of Awakening. Your years in the nurturing hands of the Tetyayath have
come to an end.

Now you must go out into the world and find your destiny...

The Awakening
  You stand next to the great old trunk of Cuivealda. The light is dim, as
the dense, high canopy that forms Tauremar covers the sky. There is little
ground cover, and the forest floor is a desolate place of perpetual
twilight.
You have no unread notes.
```

The first thing an elf character sees on this MUD now is a narrative introduction, and a framing story to guide them through the rest of the "MUD School" replacement.

But there needs to be more. I need an "OOC" call-out telling them what to do next. 

#### Delayed MobProgs

There is some text I would like to display just after the room description; but the `GREET` trigger won't be triggered because we didn't "move" into the room, and the `ACT` trigger is resolved _before_ the room is shown to the PC (which I have taken advantage of).

I will use a `DELAY` trigger.

First, I need to modify the existing MobProg:

```
mpedit 12000
code
```

...and append the following lines:

```
mob remember $n
mob delay 1
```

`MOB REMEMBER` makes the mob stow away the trigger activator's name for later. Future triggers can pull it out and look for it.

`MOB DELAY` creates a secondary trigger after the specified "mob pulses", which are approximately a fourth-of-a-second. I selected `1` (because `0` doesn't work) so that it occurs as soon after the prompt as possible.

I will create a Mob Prog reset with the `DELAY` trigger; but first I need the Mob Prog itself:

```
mpedit create 12001
code
```

...and this is the code I enter:

```
if hastarget $i
    mob echoat $q {_You have instructions to meet your old mentor in a clearing just to the east. There you{/will continue your training. Type '{*EXITS{_' to see what lies in that direction. Type '{*EAST{_'{/to go there.{x
else
    mob forget
endif
```

Note that the `MOB ECHOAT` is one lone line. MobProgs are very rudimentary. I may address that in the future, but for now I will work with what I have.

The first line demonstrates MobProgs simple `IF...Else...` logic. There is no "elif" logic. This version of MobProgs actually descends from a more capable, but much _buggier_ Mob Progs. This stripped down version is straightforward and fool-proof.

> However, I now remember back in my Imp days, crafting many tiers of nested `IF`s with a final line of `ENDIF ENDIF ENDIF ENDIF ENDIF`. I think making a basic `ELIF` would be my first Mob Progs extension.

`HASTARGET $i` is a simple test that checks to see if the name stored in the mob's memory belongs to anyone it can see. If it does, it performs `MOB ECHOAT $q`. Last time, we used `$n` to mean the person that triggered the Mob Prog. But in this case, the trigger was a `DELAY` timer. `$q` uses the name "remembered" by the mob.

And if there's no such person, the mob forgets them.

Finally I add the Mob Prog as a new reset:

```
medit 12000
addprog 12001 delay 100
done
asave changed
```

Here are the MobProgs for the room, now:

```
MOBPrograms for [12000]:
 Number Vnum Trigger Phrase
 ------ ---- ------- ------
[    0] 12001   DELAY 100
[    1] 12000     ACT has entered the game
```

And how does it work? I log in with a new elf character and test it:

```
Welcome to Mud98. Please do not feed the mobiles.

You step off the end of an old rope ladder at the base of Cuivealda, the
Tree of Awakening. Your years in the nurturing hands of the Tetyayath have
come to an end.

Now you must go out into the world and find your destiny...

The Awakening
  You stand next to the great old trunk of Cuivealda. The light is dim, as
the dense, high canopy that forms Tauremar covers the sky. There is little
ground cover, and the forest floor is a desolate place of perpetual
twilight.
You have no unread notes.

<20hp 100m 100mv>
You have instructions to meet your old mentor in a clearing just to the east. There you
will continue your training. Type 'EXITS' to see what lies in that direction. Type 'EAST'
to go there.
```

The intro text and the OOC quest text are distinctively colored and useful, and I didn't have to contrive adding them to room descriptions like MUD school did. 

## Quest-driven expansion

I've already mentioned a clearing to the east. Now I need to make it. Here's a wonderful shorthand in OLC for building out areas:

```
redit 12000
east dig 12005
```

This creates a new room (VNUM `12005`) and creates a two-way, east-west link between them. After "digging", you are now in the new room, editing it. 

> IMXP, the best way to build an area is to sketch it out first, assigning VNUMs ahead of time. I space VNUM's by 5, but if rooms will be dense, I space by 10. This lets me group obj and mob VNUMs by room in a way that is predictable and easy to manage.
>
> Sketch out the area as uniform blocks. Even in my MUD days (late 90's) auto-mappers were ubiquitious, and players were more partial to area layouts that didn't muck with their maps.

Set the new room's attributes as we did before, keeping in mind this is a clearing and there will be a quest mob, here.

### Making the quest mob

Since the room is VNUM `12005`, I'll make the quest mob have the same VNUM:

```
medit create 12005
name findorian woodspeaker
level 20
race elf
act sentinel
off assist_race
res weapon bash
short Findorian, the Woodspeaker

long Findorian, the Woodspeaker, kneels in the grassy clearing, gazing intently at
the dark woods surrounding you.
```

His stats are pretty bland. You can try to figure out what his hitroll and dice, and all that should be, or you can set them to sensible defaults with:

```
recval
```

I don't know what that means. But it uses a lookup table in `tables.c` to set default values that the entire MUD, for the most part, adheres to. You can tweak to your preference.

If you want to see these combat ratings on a sample of other mobs in the world, you can use the `MOBLIST` command (not to be confused with `MLIST`, which is only in OLC and is more bare-bones).

Once you have the mob where you want him, you can set him in place:

```
done
redit 12005
mreset 12005 1 1
done
asave changed
```

Always save your work.

### Creating the player's first quest

The first quest the elf player will receive is to go see Findorian, who will dispense the next set of quests. Since our first triggerbot will initiate the first quest, I'll use that VNUM:

```
qedit create 12000
```

This creates a blank slate of a quest:

```
New quest created.
VNUM:       [12000]
Name:       (none)
Area:       Faladrin Forest
Type:       [visit_mob]
Level:      [0]
End:        [0] (none)
Target:     [0] (none)
XP:         [0]
Entry:      [none]
```

I then fill in some information:

```
name An Old Friend
level 1
end 12005
target 12005
xp 100
```

I also add text for the quest log entry with the `ENTRY` command. Here;s what it looks like when finished:

```
VNUM:       [12000]
Name:       An Old Friend
Area:       Faladrin Forest
Type:       [visit_mob]
Level:      [1]
End:        [12005] Findorian, the Woodspeaker
Target:     [12005] Findorian, the Woodspeaker
XP:         [100]
Entry:
Before you step out into the greater world, you must be trained.  To that
end, your old mentor Findorian has has taken the responsibility to ensure
you are properly made ready.  He waits for you just to the east of
Cuivealda.  
```

The default type is `visit_mob`, which is just right for us right now. I need to go back and edit MobProg `12001` (the 1 pulse delayed action). This is the new, updated code:

```
if hastarget $i
    mob echoat $q {jYou have instructions to meet your old mentor in a clearing just to the east. There you will continue your training. Type '{*EXITS{j' to see what lies in that direction. Type '{*EAST{j' to go there.{x
    if canquest $q 12000
        mob quest grant $q 12000
        mob echoat $q {jType '{*QUEST{j' to view your quest log.{x
    endif
else
    mob forget
endif
```

The quest system in Mud98 is a novel addition of my own. There is an existing quest system for ROM that was fairly popular back in the day, but it relies on hard-coded quests and had no OLC support. Mud98's quest system is entirely OLC-driven.

In this example, the `canquest` condition checks to see if the player is eligible for the quest (at the time of this writing, this check is a level limit and an "already-completed" check). If eligible, the player is "granted" the quest, and it is now active for them. 

This is what the player sees:

```
You have instructions to meet your old mentor in a clearing just to the east.
There you will continue your training. Type 'EXITS' to see what lies in that 
direction. Type 'EAST' to go there.
You have started the quest, "An Old Friend".
Type 'QUEST' to view your quest log.
```

Here is what they see when they type `QUEST`:

```
Active Quests in Faladrin Forest:

1. An Old Friend [Level 1]
Before you step out into the greater world, you must be trained.  To that
end, your old mentor Findorian has has taken the responsibility to ensure you
are properly made ready.  He waits for you just to the east of Cuivealda.
```

This list will obviously grow as the player progresses. World quests (those outside their immediate area) will be displayed as a collapsed list below. Completed quests are hidden.

### Finishing the quest

Findorian (mob VNUM `12005`) is our quest handler. We designated him both as the quest "target" and quest "end".  We need a new Mob Prog to receive and handle the quest:

```
mpedit create 12005
code

if canfinishquest $n 12000
    say There you are, $n; I'm glad to see you. I wish I could ease you into the
brutal World Below, but we have no such luck.
    mob echo $I sighs.
    say I need your help with something, and it's not going to be pleasant.
    mob quest finish $n 12000
endif
@
done
```

I then assign this Mob Prog with a `GREET` trigger:

```
medit 12005
addprog 12005 greet 100
done
asave changed
```

So, how did we do? This is what a new character sees when they enter the room with Findorian:

```
The Training Ground
  Dappled sunlight breaks through the canopy in this clearing.  A patch of
grass grows, green and vibrant, in the center.  Dark pathways through to
trees go out in all directions.  
[?] Findorian, the Woodspeaker, kneels in the grassy clearing, gazing intently
at the dark woods surrounding you.
Findorian, the Woodspeaker says 'There you are, Elithir; I'm glad to see you. I
wish I could ease you into the brutal World Below, but we have no such luck.'
Findorian, the Woodspeaker sighs.
Findorian, the Woodspeaker says 'I need your help with something, and it's not
going to be pleasant.'
You have completed the quest, "An Old Friend".
You have been awarded 100 xp.
```

The player has now completed their first quest. I could have done a more "automated" way of handling quests that didn't involve Mob Progs, but I believe this method lets forces builders to create a more customized and narrative experience around levelling.

The `[?]` symbol in front of Findorian's name is a short-hand from MMO's meaning "this person is ready to complete your quest." In this case, the process was automated. Other quests may require some interaction. It's up to you.

### Newbie's first grind

In "MUD School", a PC needed to go kill a bunch of goblins in a pen for XP. The goblins were weak and cowering, and it was very difficult for the PC to get injured (it could happen, it was just hard). The idea was that you had a safe way to practice initiating combat without the danger of dying at level 1. It was pretty contrived, but it served its purpose.

Since the intention of this brand-new newbie area is to perform the same role, but in a more narrative fashion, I need to come up with a story and a quest for it.

So here it is: something is poisoning the forest, and a number of animals are getting sick and dying from it. The "what" is for a later quest, but the first one is conservation: humanely (elfly?) cull the sick animals. 

##### The quest area

I'll start by making a 3x3 grid to the.... north? Sure. We'll say north. A darkened wood littered with sick animals.

The task of crafting the 3x3 area is left as an exercise to the implementor. Since these rooms will share the same mob, I won't space out the VNUMs for them. I'll use VNUMs `12010`-`12019`. The shape and exist arrangements are unimportant, so long as we don't mess with players' automappers.

Here's my dig commands:

```
redit 12005
north dig 12010
west dig 12011
north dig 12012
north dig 12013
east dig 12014
east dig 12015
south dig 12016
south dig 12017
```

To get back to the entrance, I use `LINK` instead of `DIG`:

```
west link 12010
west
redit 12010
north dig 12018
west link 12012
north link 12014
east link 12016
```

Hopefully at this point, it's pretty clear the advantage that comes with pre-mapping (in 1998 I used graph paper), especially for keeping room VNUM's straight.

Here is the layout, so far:

```
    +-------+   +-------+   +-------+
    | 12013 |---| 12014 |---| 12015 |
    +---+---+   +---+---+   +---+---+
        |           |           |    
    +---+---+   +---+---+   +---+---+
    | 12012 |---| 12018 |---| 12016 |
    +-------+   +---+---+   +---+---+
        |           |           |   
    +---+---+   +---+---+   +---+---+
    | 12011 |---| 12010 |---| 12017 |
    +-------+   +---+---+   +-------+
                    |
    +-------+   +---+---+
    | 12000 |---| 12005 |
    +-------+   +-------+
```

In the first room, I create this sickened creature:

```
Name:        [sick deer]
Vnum:        [ 12010]
Area:        [    48] Faladrin Forest
Level:       [     1] Sex:     [  none] Group:   [    0]
Align:       [     0] Hitroll: [     0] Dam type: [none]
Hit dice:    [  1d1  +   7] Damage dice:  [  1d2  +   0]
Mana dice:   [  1d10 + 100] Material:     [     unknown]
Race:        [        fido] Size:         [      medium]
Start pos.:  [    standing] Default pos.: [    standing]
Wealth:      [    0]
Armor:       [pierce: 100  bash: 100  slash: 100  magic: 100]
Affected by: [none]
Act:         [npc sentinel wimpy]
Form:        [edible poison animal mammal]
Parts:       [head legs heart brains guts ear eye tail horns]
Imm:         [summon charm]
Res:         [none]
Vuln:        [magic]
Off:         [none]
Short descr: A sick deer
Long descr:
A sick deer drags a limp leg along the ground.
Description:
This deer has been poisoned, and is near death.  Its eyes are bloodshot
and crazed.  It eyes you warily. 
```

Then we add it to room `12010`'s resets:

```
mreset 12010 1 1
```

##### Cloning mobs

I want each room in the Poisoned Woods to be a unique creature, but I don't want to have to actually create them all one at a time. Thanks to `COPY`, I don't have to:

```
medit create 12011
copy 12010
```

All I have to do is change the name and descriptions and tweak the combat values, and I'm good to go. I repeat this process for all nine rooms, making them all very easy. Some I leave off `wimpy`, and others I add `aggressive`. Some attack with `1d2` damage, and some do `1d4` (in line with MUD school).

Here's the results of `MOBLIST AREA` after adding the creatures:

```
VNUM   Name       Lvl Hit Dice     Hit   Dam      Mana       Pie  Bas  Sla  Mag
===============================================================================
12000  (no short  0   0d0+0        0     0d0+0    0d0+0        0    0    0    0
12005  Findorian, 20  3d9+333      0     2d8+5    20d12+100  -40  -40  -40  -30
12010  A sick dee 1   1d1+7        0     1d2+0    1d10+100   100  100  100  100
12011  A blighted 1   1d1+7        0     1d2+0    1d10+100   100  100  100  100
12012  A rabid bo 1   1d1+7        0     1d3+0    1d10+100   100  100  100  100
12013  A crazed b 1   1d1+7        0     1d4+0    1d10+100   100  100  100  100
12014  A wild wea 1   1d1+7        0     1d3+0    1d10+100   100  100  100  100
12015  A mutant c 1   1d1+7        0     1d4+0    1d10+100   100  100  100  100
12016  An angry b 1   1d1+7        0     1d3+0    1d10+100   100  100  100  100
12017  A feral ra 1   1d1+7        0     1d2+0    1d10+100   100  100  100  100
12018  An agonize 1   1d1+7        0     1d2+0    1d10+100   100  100  100  100
```

These stats are in line with the traditional MUD school monsters.

#### Creating the quest

Findorian will be dispensing this quest, so it will follow his VNUM list:

```
qedit create 12006
name A Preserver's Burden
type kill_mob
end 12005
xp 200
level 1
target 12010
amount 9
```

This quest uses a couple values we didn't have, before, which means `target` has a very different meaning. Before, for `visit_mob`, Findorian was both the `end` and the `target`. Now he is the `end`, and the "sick deer" is the target. You can specify the number of times the quest conditions (as defined by the quest type) must be performed before the quest is "finished" and can be turned in. In this case, the number of times the "sick deer" must be killed.

Here is the quest as it stands, now:

```
VNUM:       [12006]
Name:       A Preserver's Burden
Area:       Faladrin Forest
Type:       [kill_mob]
Level:      [1]
End:        [12005] Findorian, the Woodspeaker
Target:     [12010] A sick deer (set UPPER to use a range of VNUMs)
Amount:     [9]
XP:         [200]
Entry:      [none]
```

Now, we created nine unique monsters for this quest, and what we really want is for each of these mobs to be a possible fulfillment of the quest. As the suggestion text for `target` says above, we do so with `upper`:

```
upper 12018
```

This creates an upper-bound on a range of VNUMs that are now valid targets for this quest:

```
VNUM:       [12006]
Name:       A Preserver's Burden
Area:       Faladrin Forest
Type:       [kill_mob]
Level:      [1]
End:        [12005] Findorian, the Woodspeaker
Target:     [12010] A sick deer
            [12011] A blighted fox
            [12012] A rabid bobcat
            [12013] A crazed black bear
            [12014] A wild weasel
            [12015] A mutant coyote
            [12016] An angry badger
            [12017] A feral rabbit
Upper:      [12018] An agonized boar
Amount:     [9]
XP:         [200]
Entry:      [none]
```

Add an entry to show up in the quest log, and save the changes.

Now I need to extend Findorian's existing Mob Prog #`12005`:

```
if canfinishquest $n 12000
    say There you are, $n; I'm glad to see you. I wish I could ease you into the brutal World Below, but we have no such luck.
    mob echo $I sighs.
    say I need your help with something, and it's not going to be pleasant.
    mob quest finish $n 12000
    say There is a blight in this region of the forest infecting flora and fauna alike. I want to get to the root of it, but for now there is great suffering that must be dealt with.
    say This is the burden of a Forestspeaker; there is only one way to stop the spread of the disease and bring peace to the afflicted, and that is by death.
    say The Tetyayath have taught you the Creeds, and you have spoken them. Now you must pay the terrible cost of adhering to them.
    say Go north. Bring them mercy. They have suffered long enough.
    mob quest grant $n 12006
endif
if canfinishquest $n 12006
    say Bloody business, that. I'm sorry.
    mob echo $I places his hand on $s shoulder.
    say What you have done here important. Preservation is about protecting what we can, and mitigating when we can't.
    say You have honored the Creeds. You have honored the Forest.
    mob quest finish $n 12006
endif
```

Notice that the same trigger is now driving two different quests. I write quests in such a way that multiple quests can be turned in at the same time. It would be strange to have to leave, and reenter the room to fulfill a second quest.

### Testing

Here's how the first completion looks with the second quest appended:

```
The Training Ground
  Dappled sunlight breaks through the canopy in this clearing.  A patch of
grass grows, green and vibrant, in the center.  Dark pathways through to
trees go out in all directions.  
  [?] Findorian, the Woodspeaker, kneels in the grassy clearing, gazing intently
at the dark woods surrounding you.
Findorian, the Woodspeaker says 'There you are, Elithir; I'm glad to see you. I
wish I could ease you into the brutal World Below, but we have no such luck.'
Findorian, the Woodspeaker sighs.
Findorian, the Woodspeaker says 'I need your help with something, and it's not
going to be pleasant.'
You have completed the quest, "An Old Friend".
You have been awarded 100 xp.
Findorian, the Woodspeaker says 'There is a blight in this region of the forest
infecting flora and fauna alike. I want to get to the root of it, but for now 
there is great suffering that must be dealt with.'
Findorian, the Woodspeaker says 'This is the burden of a Forestspeaker; there is
only one way to stop the spread of the disease and bring peace to the afflicted, 
and that is by death.'
Findorian, the Woodspeaker says 'The Tetyayath have taught you the Creeds, and
you have spoken them. Now you must pay the terrible cost of adhering to them.'
Findorian, the Woodspeaker says 'Go north. Bring them mercy. They have suffered
long enough.'
You have started the quest, "A Preserver's Burden".
```

It's a wall of text in monochrome, but with color themes enabled, it is broken up into visually distinct sections.

When we go north, we see this:

```
The Poisoned Woods
  There is only a glimmer of light from the clearing to the south. 
Otherwise, the forest floor is almost completely obscured by the high canopy
of trees.  You can see signs of some sort of blight that is infecting plant
and animal alike.  Tree trunks are turning ashen white, and there are
carcasses of hairless creatures strewn about.  The sickened forest extends
in all directions.  
  [X] A sick deer drags a limp leg along the ground.
```

The `X` indicates the mob is a "kill" target for a quest. When you kill the deer, you get this message:

```
A sick deer is DEAD!!
You receive 148 experience points.
Quest Progress: A Preserver's Burden (1/9)
A sick deer spills its guts all over the floor.
```

### Wait, I messed up; back up

That fight took _forever_. Why? Because my character had 1% in all skills and base ability scores. Also, no gear. That won't do.

In MUD school, you `TRAIN` and `PRACTICE` before you fight. So I'm going to splice in some rooms between 12000 and 12005 to equip and train the player.

Here's how I create the new layout:

```
redit 12000
east delete
east dig 12002
east dig 12003
east link 12005
```

I started with 12002 because MobProg #`12001` is already used by the triggerbot in room `12000`. As I said, I like to at least _try_ to keep my VNUMs in sync. The decision to start the second room at VNUM 12005 turns out to be wise, in retrospect, as (as I predicted) I might have needed to insert a room or two inbetween.

We can successively dig serially like this because each `dig` moves us into the new room, and sets that as our edit point.

#### The training room

The first room will serve two purposes: to teach the player how to `READ` notes, and how to `TRAIN` and `PRACTICE`.

```
redit 12002
name A Dark Path Through the Woods
flags no_recall
desc
This path through the forest is just out of sight of Cuivealda to the
west.  The trees are densely packed on all sides except on the path, which
continues east.  
  There is a hand-written note posted to a tree.
@
```

The note is a piece of scenery; the player can't take it with them. Rather, it's an "extra description" with a long-form editor we invoke like so:

```
ed add note
Before you come see me, take some time and center yourself.
Be sure to {*TRAIN{x your abilities and {*PRACTICE{x your skills.
Never be afraid to seek {*HELP{x if you lack understanding.
 
--F
```

Now, in my example, I have the config settings `train_anywhere` and `practice_anywhere` enabled. If you don't, you need to maybe need to move this part over to Findorian and add the `train` and `practice` flags to him, or add them to the triggerbot we're about to create.

To let the player know to look at the note, I want to do an OOC announcement with a triggerbot. First, the Mob Prog:

```
mpedit create 12002
code
if hasquest $n 12000
    mob echoat $n {jTo read the note tacked to the tree, type any of the
following:{j
    mob echoat $n {*READ NOTE{j, {*LOOK NOTE{j, or {*EXAMINE NOTE{j.
endif
@
done
```

Now I make a new triggerbot, attach a new mob prog to it, and load it:

```
medit create 12002
copy 12000
name triggerbot_12002
addprog 12002 greet 100
done
redit 12002
mreset 12002 1 1
done
asave changed
```

And with that, the player is now empowered to train abilities and practice skills.

#### The equipping room

To further get parity with the old MUD school, we need to get the player their starting equipment. Right now, all the starting equipment is from MUD school, which is a "sub-standard issue" set.

We can do better.

First, we need to create brand new starting equipment; we don't want any dependency on ROM's MUD school, but we _can_ use it as a reference:

```
goto 3700
objlist area armor
```

This gives us the values for the starting gear:

```
VNUM   Lvl Name                AC:  Pierce  Bash    Slash   Exotic        
===========================================================================
3703   0   a sub issue vest         [    1] [    1] [    1] [    0] [    0] 
3704   0   a sub issue shield       [    1] [    1] [    1] [    0] [    0] 
3705   0   a sub issue cloak        [    1] [    1] [    1] [    0] [    1] 
3706   0   a sub issue helmet       [    1] [    1] [    1] [    0] [    0] 
3707   0   a pair of sub issue leg  [    1] [    1] [    1] [    0] [    0] 
3708   0   a pair of sub issue boo  [    1] [    1] [    1] [    0] [    0] 
3709   0   a pair of sub issue glo  [    1] [    1] [    1] [    0] [    0] 
3710   0   a pair of sub issue sle  [    1] [    1] [    1] [    0] [    0] 
3711   0   a sub issue cape         [    1] [    1] [    1] [    0] [    0] 
3712   0   a sub issue belt         [    1] [    1] [    1] [    0] [    0] 
3713   0   a sub issue bracer       [    0] [    1] [    1] [    0] [    0] 
```

It's just `1`'s across the board for AC.

Right now, there aren't any objects listed in Faladrin Forest, so we can start at `12000`:

```
oedit create 12000
type armor
v0 1
v1 1
v2 1
wear take body
materal cloth
name shirt linen cuivealda
short a linen shirt of Cuivealda
long A linen shirt of Cuivealda lays on the ground.
done
```

Most of these settings will be the same across items:

```
oedit create 12001
copy 12000
wear body neck
name cloak linen cuivealda
short a linen cloak of Cuivealda
long A linen cloak of Cuivealda is discarded here.
done
```

The second `wear` line needs some explaining: in Mud98, each word fed into the OLC flags toggle is parsed as an individual flag and toggled on its own. In this case, `body` is toggled OFF and `neck` is toggled ON. The `take` flag is left alone.

I continue in this way until I have a complete replacement for all the starter gear, and my `OBJLIST AREA ARMOR` in Faladrin Forest looks like this:

```
VNUM   Lvl Name                AC:  Pierce  Bash    Slash   Exotic        
===========================================================================
12000  0   a linen shirt of Cuivea  [    1] [    1] [    1] [    0] [    0] 
12001  0   a linen cloak of Cuivea  [    1] [    1] [    1] [    0] [    0] 
12002  0   a linen cap of Cuiveald  [    1] [    1] [    1] [    0] [    0] 
12003  0   linen pants of Cuiveald  [    1] [    1] [    1] [    0] [    0] 
12004  0   wooden clogs of Cuiveal  [    1] [    1] [    1] [    0] [    0] 
12005  0   simple hide gloves       [    1] [    1] [    1] [    0] [    0] 
12006  0   a belt of hemp rope      [    1] [    1] [    1] [    0] [    0] 
12007  0   simple hempen wristband  [    1] [    1] [    1] [    0] [    0] 
```

Now I make a chest to put all this stuff in:

```
oedit create 12008
type container
material wood
v0 100
v1 closed closeable
v3 8
extra nopurge
name chest oaken stout
short a stout oaken chest
long A stout oaken chest is nestled between the roots a great tree trunk.
```

And now I load up the trunk with the items:

```
redit 12003
name A Dark Path Through the Woods
desc
The path through the dark woods continues.  The way west leads back to
Cuivealda, while the path continuing east leads to sunlight.
@
oreset 12008
oreset 12000 chest
oreset 12001 chest
oreset 12002 chest
oreset 12003 chest
oreset 12004 chest
oreset 12005 chest
oreset 12006 chest
oreset 12007 chest
```

The room should now look like this:

```
Description:
The path through the dark woods continues.  The way west leads back to
Cuivealda, while the path continuing east leads to sunlight.  
Name:       [A Dark Path Through the Woods]
Area:       [   48] Faladrin Forest
Vnum:       [12003]
Sector:     [forest]
Room flags: [no_recall]
Heal rec:   [100]
Mana rec:   [100]
Characters: [halivar]
Objects:    [ chest]
Exits:
    East :  [12005] Key: [    0] Exit flags: [none]
    West :  [12002] Key: [    0] Exit flags: [none]
Resets: M = mobile, R = room, O = object, P = pet, S = shopkeeper
 No.  Loads    Description       Location         Vnum   Ar Rm Description
==== ======== ============= =================== ======== ===== ===========
[ 1] O[12008] a stout oaken                     R[12003]       A Dark Path Thr
[ 2] O[12000] a linen shirt inside              O[12008]  0- 1 a stout oaken c
[ 3] O[12001] a linen cloak inside              O[12008]  0- 1 a stout oaken c
[ 4] O[12002] a linen cap o inside              O[12008]  0- 1 a stout oaken c
[ 5] O[12003] linen pants o inside              O[12008]  0- 1 a stout oaken c
[ 6] O[12004] wooden clogs  inside              O[12008]  0- 1 a stout oaken c
[ 7] O[12005] simple hide g inside              O[12008]  0- 1 a stout oaken c
[ 8] O[12006] a belt of hem inside              O[12008]  0- 1 a stout oaken c
[ 9] O[12007] simple hempen inside              O[12008]  0- 1 a stout oaken c
```

Now if you log into the game with a newbie char, you can go up to the chest and perform the following commands:

```
open chest
look in chest
```

Which produces the following:

```
A stout oaken chest holds:
     simple hempen wristbands
     a belt of hemp rope
     simple hide gloves
     wooden clogs of Cuivealda
     linen pants of Cuivealda
     a linen cap of Cuivealda
     a linen cloak of Cuivealda
     a linen shirt of Cuivealda
```

Because we enabled `alwaysreset` on the area, this chest will replenish its items every 8 minutes. 

Now I want to make am Mob Prog and a triggerbot to let the player know what to do, here.

```
mpedit create 12003
code
if hasquest $n 12000
    mob echoat $n {jYour mentor has left you some gear to help you on 
your{x
    mob echoat $n {jjourney. It's inside a chest by the tree.{x
    mob echoat $n {jTry the following commands:{x
    mob echoat $n     {*OPEN CHEST{x
    mob echoat $n     {*LOOK IN CHEST{x
    mob echoat $n     {*TAKE ALL FROM CHEST{x
    mob echoat $n     {*WEAR ALL{x
    mob echoat $n {jYou can also look at your inventory with:{x
    mob echoat $n     {*EQ{j (short for {*EQUIPMENT{j){x
    mob echoat $n     {*INV{j (short for {*INVENTORY{j){x
    mob echoat $n {jYou can see the effect donning this gear by viewing your stats:{x
    mob echoat $n     {*SCORE{j (which you can abbreviate to {*SC{j){x}
endif
@
done
medit create 12003
copy 12000
name triggerbot_12003
addprog 12003 greet 100
done
redit 12003
mreset 12003 1 1
done
asave changed
```

Now if we load into a new character, we can see the triggerbots in action; prompting us and letting our newbie know how to proceed.

Now, with all of our equipment, we can finish testing the `kill_mob` quest.

### Completing newbie's first grind

Continuing with our newbie char, we embark on Findorian's quest and start killing the deranged woodland creatures. We start seeing text like this:

```
A blighted fox is DEAD!!
You receive 202 experience points.
Quest Progress: A Preserver's Burden (2/9)
You hear A blighted fox's death cry.
```

The quest is ready to turn in when we see this:

```
A mutant coyote is DEAD!!
You receive 274 experience points.
Quest Progress: A Preserver's Burden (9/9)
A mutant coyote hits the ground ... DEAD.
```

We've hit 9 of the 9 alotted targets, and we will no longer accrue any more. We are ready to turn in the quest to Findorian:

```
Findorian, the Woodspeaker says 'Bloody business, that. I'm sorry.'
Findorian, the Woodspeaker places his hand on his shoulder.
Findorian, the Woodspeaker says 'What you have done here important. Preservation
is about protecting what we can, and mitigating when we can't.'
Findorian, the Woodspeaker says 'You have honored the Creeds. You have honored
the Forest.'
You have completed the quest, "A Preserver's Burden".
You have been awarded 180 xp.
```

With just those two quest types, we can do a lot of narrative world expansion. The task of completing this area, and of making other racial (or even class) starting areas is up to you.