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

### Creating a new area

Before creating your new area, type `HELP AEDIT` and get a rough idea of the information you will need. To get started, simply type this:

```
aedit create
```

...which will bring up this blank slate of information:

```
Name:       [48] New area
File:       area48.are
Vnums:      [0-0]
Age:        [0]
Security:   [9]
Builders:   [Halivar]
Credits :   [Halivar]
Min_level:  [1]
Max_level:  [60]
Flags:      [added]
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
vnums 12000 121000
max_level 5
```

Now my `SHOW` looks like this:

```
Name:       [48] Faladrin Forest
File:       faladri.are
Vnums:      [12000-12100]
Age:        [0]
Security:   [9]
Builders:   [Halivar]
Credits :   [Halivar]
Min_level:  [1]
Max_level:  [5]
Flags:      [changed added]
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

The command for editing rooms is `REDIT`, and it always helps to type `HELP REDIT` before you use it. Creating almost as simple as creating an area; all we need is the starting VNUM:

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
Sector:     [inside]
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

The "sector" is currently set to `inside`. Type `SECTOR` to get a list of available options.  I'm putting mine in a forest (obviously):

```
sector forest
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

Notice under `Start Loc`, it says "(not used)". That means Mud98 isn't configured to use racial start locations. Be default, all non-stock-ROM behavior is disabled to provide a more authentic, legacy ROM experience out-of-the-box.

Open up `mud98.cfg` in the root directory and change this setting (uncommenting it, if needed):

```
start_loc_by_race = yes
```

Note that this is a completely safe flag; it has no effect on races that don't have a start location set, yet.

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
    Mage        100     1000( 40)    [    0] The Awakening  (not used)
    Cleric      125     1000( 40)    [    0] The Awakening  (not used)
    Thief       100     1000( 40)    [    0] The Awakening  (not used)
    Warrior     120     1000( 40)    [    0] The Awakening  (not used)

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

The `mob echoat $n` command says that the mob executing the script will perform an "echo" (unquoted text) directly to the character that prompted the trigger. I added coloration to add some flair as a call-out. The `{/` color code is actually a new-line. When do doing mobprogs like this, you will need to keep track of your own line endings.

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

... and this is the code I enter

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