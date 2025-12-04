# Worldcrafting from Scratch Pt. 2 &mdash; New Beginnings

Previous: [Worldcrafting from Scratch Pt. 1 &mdash; Getting Started](wb-01-getting-started)

### Table of Contents
- [Worldcrafting from Scratch Pt. 2 — New Beginnings](#worldcrafting-from-scratch-pt-2--new-beginnings)
    - [Table of Contents](#table-of-contents)
  - [Choosing a VNUM block](#choosing-a-vnum-block)
  - [Creating a new area](#creating-a-new-area)
  - [Creating the starting room](#creating-the-starting-room)
  - [Setting racial starting location](#setting-racial-starting-location)
  - [Creating a greeting for new characters](#creating-a-greeting-for-new-characters)
    - [Making a triggerbot](#making-a-triggerbot)
    - [Creating a Mob Prog](#creating-a-mob-prog)
    - [Making the triggerbot trigger](#making-the-triggerbot-trigger)
    - [Adding triggerbot to the room](#adding-triggerbot-to-the-room)
    - [Testing the triggerbot](#testing-the-triggerbot)
    - [Delayed MobProgs](#delayed-mobprogs)
  - [Quest-driven expansion](#quest-driven-expansion)
    - [Making the quest mob](#making-the-quest-mob)
    - [Creating the player's first quest](#creating-the-players-first-quest)
    - [Finishing the quest](#finishing-the-quest)
  - [Newbie's first grind](#newbies-first-grind)
    - [The quest area](#the-quest-area)
    - [Cloning mobs](#cloning-mobs)
    - [Creating a grind quest](#creating-a-grind-quest)
    - [Testing](#testing)
  - [Adding a tutorial](#adding-a-tutorial)
    - [The training room](#the-training-room)
    - [The equipping room](#the-equipping-room)
  - [Completing newbie's first grind](#completing-newbies-first-grind)

<br />

Let's start with a brand new area. This can be the seed of a brand new network
of areas that will define _your_ MUD. For the purposes of this example, we'll make this a new starting area for one of the races.

<div style="background-color: rgb(32,64,255,0.1);border-radius:5px;padding: 1em; margin-bottom: 1em;"><p style="margin: 0">Why? Because while MUDs began as heavily tilted toward hack n' slash, most players want some measure of narrative immersion that isn't possible in stock ROM. So, instead of dropping new players into "MUD School", it's better to drop them into an area that gradually introduces them to the world (and the game mechanics).</p></div>

Before we create this new area, we need to find a block of VNUMs that are free to use. 

## Choosing a VNUM block

What's a VNUM? A **VNUM** is just a number identifying each unique in-game entity definition. Rooms, objs, and mobs all have VNUMs. They can overlap _across_ entity type, but not _within_.

That is to say, you can have a `RoomData` with VNUM `1000`, an `ObjectPrototype` with VNUM `1000`, and a `MobPrototype` with VNUM `1000`. But you _cannot_ have two `RoomData`'s with the same VNUM.

Areas are defined by blocks of VNUMs that are guaranteed (by convention and bounds checking) to belong to a single area "file". For instance, "Limbo" is defined in `limbo.are` as covering VNUMs from `1` to `30`. Nothing in the game, whether a room, a mob, or even a mob prog, should exist with a VNUM in that space except insofar as it belongs to, and is defined by, `limbo.are`.

The VNUM range is a _reservation_; by declaring it upon area creation, you guarantee that you have future expansion potential _at least_ up to that end VNUM for the area.

So the first thing to decide is what the VNUM range of a new area will be.

Enter this command:

```c
alist orderby vnum
```

You should see a list that looks like this, showing VNUMs in order, and displaying any gaps in between:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(245,216,147);">Num</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(245,216,147);">Area Name             </span><span style="color: rgb(64,35,30);">] (</span><span style="color: rgb(245,216,147);"> lvnum</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(245,216,147);">uvnum </span><span style="color: rgb(64,35,30);">) [</span><span style="color: rgb(245,216,147);">Filename   </span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(245,216,147);">Sec </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(245,216,147);">Builders  </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 26</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Limbo                    </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">     1</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">30    </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">limbo.are     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 43</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Smurfville               </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">   100</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">199   </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">smurf.are     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  4</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Plains                   </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">   300</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">399   </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">plains.are    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  1</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> New Ofcol                </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">   600</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">699   </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">ofcol2.are    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 37</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Olympus                  </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">   900</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">999   </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">olympus.are   </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  6</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> In the Air               </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1000</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1099  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">air.are       </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  2</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Shire                    </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1100</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1199  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">shire.are     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  0</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Valhalla                 </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1200</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1299  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">immort.are    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 24</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> High Tower               </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1300</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1499  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">hitower.are   </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 21</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Gnome Village            </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1500</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1599  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">gnome.are     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 47</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Wyvern's Tower           </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  1600</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">1799  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">wyvern.are    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 10</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Catacombs                </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  2000</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">2099  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">catacomb.are  </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 25</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Gangland                 </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  2100</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">2199  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">hood.are      </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 13</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Dragon Tower             </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  2200</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">2299  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">draconia.are  </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 27</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Mahn-Tor                 </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  2300</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">2399  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">mahntor.are   </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 45</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Troll Den                </span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">  2800</span><span style="color: rgb(64,35,30);">-</span><span style="color: rgb(112,77,43);">2899  </span><span style="color: rgb(64,35,30);">) </span><span style="color: rgb(43,69,79);">trollden.are  </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">9</span><span style="color: rgb(64,35,30);">] [</span><span style="color: rgb(112,77,43);">          </span><span style="color: rgb(64,35,30);">]</span><br>
 </div>

 <div style="background-color: rgb(32,64,255,0.1);border-radius:5px;padding: 1em; margin-bottom: 1em;"><p style="margin: 0">This is the built-in color theme "Conestoga". It's selectable via the <code>THEME</code> command.</p></div>

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

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        48</span><span style="color: rgb(64,35,30);"> ] </span><br>
<span style="color: rgb(237,230,203);">File           : </span><span style="color: rgb(112,77,43);">area48.json</span><br>
<span style="color: rgb(237,230,203);">Vnums          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       0-0</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Levels         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">      1-60</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Sector         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    inside</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Reset          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         6</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">x 48 minutes</span><br>
<span style="color: rgb(237,230,203);">Always Reset   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        </span><span style="color: rgb(64,35,30);">NO ]</span><br>
<span style="color: rgb(237,230,203);">Instance Type  : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    single</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Security       : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         9</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Builders       : </span><span style="color: rgb(112,77,43);">Halivar</span><br>
<span style="color: rgb(237,230,203);">Credits        : </span><span style="color: rgb(112,77,43);">Halivar</span><br>
<span style="color: rgb(237,230,203);">Flags          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     added</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(245,216,147);">Factions</span><br>
<span style="color: rgb(43,69,79);">  (none)</span><br>
<span style="color: rgb(232,178,111);">Story Beats:</span><br>
<span style="color: rgb(237,230,203);">  (none)</span><br>
<span style="color: rgb(232,178,111);">Checklist:</span><br>
<span style="color: rgb(237,230,203);">  1) [To-Do] Write-up story beats</span><br>
<span style="color: rgb(237,230,203);">  2) [To-Do] Sketch out rooms</span><br>
<span style="color: rgb(237,230,203);">  3) [To-Do] Populate mobiles</span><br>
<span style="color: rgb(237,230,203);">  4) [To-Do] Populate objects</span><br>
<span style="color: rgb(237,230,203);">  5) [To-Do] Add descriptions</span><br>
<span style="color: rgb(237,230,203);">  6) [To-Do] Wire-up events</span><br>
 </div>

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
vnums 12000 12099
levels 1 5
reset 1
instancetype multi
```

These settings are novel to Mud98:
- `sector` sets a "default" sector value for the rooms in the area.
- `reset` is the number of "area pulses" to use before the room resets. By setting it to `1`, we guarantee this starting area (including mobs and objs) resets ever 8 minutes.
- `instancetype` is a novel setting for Mud98. There are two settings (at the
time of this writing): `single` and `multi`. The former works like ROM always
has: everyone plays in one big shared space, and players must contend with
constantly-depleted newbie areas. The `multi` option means "multiple-instanced";
each player (and their party mates) have their own sandbox in that area. If 
other folks enter the same area, they will have their _own_ instance, and the
two shall never meet. This is just right for the newbie zone, which (IMXP) can
suffer from too many folks competing for slaying the poor little moblets.

Now my `SHOW` looks like this:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        48</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">File           : </span><span style="color: rgb(112,77,43);">faladri.json</span><br>
<span style="color: rgb(237,230,203);">Vnums          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">12000-12099</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Levels         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       1-5</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Sector         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    forest</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Reset          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         1</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">x 8 minutes</span><br>
<span style="color: rgb(237,230,203);">Always Reset   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        </span><span style="color: rgb(64,35,30);">NO ]</span><br>
<span style="color: rgb(237,230,203);">Instance Type  : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     multi</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Security       : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         9</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Builders       : </span><span style="color: rgb(112,77,43);">Halivar</span><br>
<span style="color: rgb(237,230,203);">Credits        : </span><span style="color: rgb(112,77,43);">Halivar</span><br>
<span style="color: rgb(237,230,203);">Flags          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">changed added</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(245,216,147);">Factions</span><br>
<span style="color: rgb(43,69,79);">  (none)</span><br>
<span style="color: rgb(232,178,111);">Story Beats:</span><br>
<span style="color: rgb(237,230,203);">  (none)</span><br>
<span style="color: rgb(232,178,111);">Checklist:</span><br>
<span style="color: rgb(237,230,203);">  1) [To-Do] Write-up story beats</span><br>
<span style="color: rgb(237,230,203);">  2) [To-Do] Sketch out rooms</span><br>
<span style="color: rgb(237,230,203);">  3) [To-Do] Populate mobiles</span><br>
<span style="color: rgb(237,230,203);">  4) [To-Do] Populate objects</span><br>
<span style="color: rgb(237,230,203);">  5) [To-Do] Add descriptions</span><br>
<span style="color: rgb(237,230,203);">  6) [To-Do] Wire-up events</span><br>
 </div>

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">Note the file name; this guide assumes you are using JSON file formats for area and data files. If you chose not to use JSON, or chose ROM-OLC as the default in <code>mud98.cfg</code>, you would see <code>faladri.are</code> here, instead.</p>
</div>

`CREDITS` can be whatever I want (but should be my Imp name, or else it will mess with the formatting on the universal, non-OLC `AREAS` command) , but `SECURITY` and `BUILDERS` is special. When I created the area, they were set (by default) to my security level and player name, respectively. There are "permissions" for the area. To edit this area, my name must be in the `BUILDERS` list (as it can have more than one name) _or_ my security level must be higher than that of the area. Thus, an Imm who has higher security than you can edit your areas; but not lower unless they are listed as a builder in that area. As an Imp with max security, I can edit _any_ area.

### New tools for Mud98

If you've ever used OLC on a previous iteration of ROM (or other Diku derivative),
you may have noticed a few more options that are unfamiliar. These are some tools
I've put into Mud98 to help you craft a better, higher-quality, and more
consistent world:

**Factions** - NPCs in Mud98 can be assigned to factions; these are created
ad-hoc by builders, and can be constructed into an intricate web of alliances
and enemies. Players can gain (or lose) reputation with these factions by quests,
attacking their enemies (or them), or by doing to same for related factions.

**Story Beats** - This is a quick, at-a-glance list of the plots and places of 
the area, as well as significant factions and NPCs. Use this as a "build guide"
to establish themes and player-goals in areas where builders may collaborate on
a shared world.

**Checklist** - Building a quality area takes a long time; breaking down the work
into discrete chunks helps keep things straight across many building sessions,
as well as organize work across a team of builders. Pictured here is the "default"
set of builder checklist items; but a builder can clear them and create their own.
Or, an implementor can change the default list in code.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><h3 style="margin-top: 0;">Building with generative AI</h3>
<p>I am not a big fan of generative AI presented as polished human-written 
prose, because there really is no comparison. Human prose, even (or perhaps especially) badly-written, has a charm all its own that generative AI can't replicate with its sterile perfection.</p><p>Nevertheless, there is no putting
the genie back in the bottle, and I predict MUD operators in a greatly 
diminished hobby will find it harder and harder to pass up the opportunity to
instantly fill in areas; especially when standing up new MUDs <i>before</i> 
they have attracted a cadre of competent builders.</p>
<p>For these situations, <code>Factions</code>, <code>Story Beats</code>,
and <code>Checklist</code> are tools that make it trivial for agentic LLMs to
bang out some rooms, mobs, items, quests, and even Lox scripts.</p><p>But be
forewarned: it won't be <i>good</i>, per se, and many players won't appreciate 
it. Prospective builders may even be <i>insulted</i> by it.</p><p style="margin: 0;"><b>My recommendation:</b> Use generative AI on a builder-starved MUD for quick
scaffolding; but put in the extra work to improve, rewrite, and fix issues
before you present it to the public as "finished".</div>

### Crafting a narrative

I'll keep the default checklist (since I came up with them, in the first place)
and move on the story beats:

```
story add "Elf starting area" On-board new elf PCs with a story-driven tutorial.
story add "Cuivealda's shadow" The entire area is under the sprawling branches 
of the elven tree home.
story add "The Forestspeaker's plight" Findorian needs the player's help to 
fight a blight consuming the forest.
```

If you hit "Enter" again (or type `SHOW`), you'll see our list of story beats
have been added to the area.

Now we can mark our first "to-do" item complete:

```
checklist status 1 done
```

Technically, we should have marked it "In progress" before starting. Just
consider yourself lucky I didn't add a Jira ticketing process.

One more thing to do on our way out the door: I want to go ahead and set our 
initial faction:

```
faction create 12000 Tauremar
```

Now we can take a look and see our progress (initial lines of `SHOW` omitted):

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(245,216,147);">Factions</span><br>
<span style="color: rgb(64,35,30);">[12000] </span><span style="color: rgb(112,77,43);">Tauremar            </span><span style="color: rgb(237,230,203);"> default: </span><span style="color: rgb(43,69,79);">     0</span><span style="color: rgb(237,230,203);"> allies: </span><span style="color: rgb(43,69,79);"> 0</span><span style="color: rgb(237,230,203);"> enemies: </span><span style="color: rgb(43,69,79);"> 0</span><br>
<span style="color: rgb(232,178,111);">Story Beats:</span><br>
<span style="color: rgb(237,230,203);">  1) Elf starting area</span><br>
<span style="color: rgb(237,230,203);">     On-board new elf PCs with a story-driven tutorial.</span><br>
<span style="color: rgb(237,230,203);">  2) Cuivealda's shadow</span><br>
<span style="color: rgb(237,230,203);">     The entire area is under the sprawling branches of the elven tree home.</span><br>
<span style="color: rgb(237,230,203);">  3) The Forestspeaker's plight</span><br>
<span style="color: rgb(237,230,203);">     Findorian needs the player's help to fight a blight consuming the forest.</span><br>
<span style="color: rgb(232,178,111);">Checklist:</span><br>
<span style="color: rgb(237,230,203);">  1) [Done] Write-up story beats</span><br>
<span style="color: rgb(237,230,203);">  2) [To-Do] Sketch out rooms</span><br>
<span style="color: rgb(237,230,203);">  3) [To-Do] Populate mobiles</span><br>
<span style="color: rgb(237,230,203);">  4) [To-Do] Populate objects</span><br>
<span style="color: rgb(237,230,203);">  5) [To-Do] Add descriptions</span><br>
<span style="color: rgb(237,230,203);">  6) [To-Do] Wire-up events</span><br>
 </div>

### Wrapping up

Now it's time to set this area is stone by saving it:

```
done
asave changed
```

We get this message:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Saved zones:</span><br>
<span style="color: rgb(237,230,203);">         Faladrin Forest - 'faladri.json'</span><br>
 </div>

If do a `git status` in my `area` folder, I see my changed files:

```
Changes not staged for commit:
        modified:   area/area.lst

Untracked files:
        area/faladri.json
```

That's it! I now have my new starting area, and I'm read to start adding to rooms to it.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;"><p>You <i>are</i> using <code>git</code> to keep track of your changes, right?</p>
<p style="margin: 0">
Right?</p>
</div>

## Creating the starting room

The command for editing rooms is `REDIT`, and it always helps to type `HELP REDIT` before you use it. Creating a room is almost as simple as creating an area; all we need is the starting VNUM:

```
redit create 12000
```

...which gives us this:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(232,178,111);">Room created.</span><br>
<span style="color: rgb(237,230,203);">Description    : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Room           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><br>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        48</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Sector         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    forest</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Room Flags     : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Heal Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Mana Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Characters     : </span><span style="color: rgb(112,77,43);">halivar </span><br>
<span style="color: rgb(237,230,203);">Objects        : </span><span style="color: rgb(112,77,43);">(none)</span><br>
<span style="color: rgb(237,230,203);">Lox Class      : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Type '</span><span style="color: rgb(232,178,111);">LOX</span><span style="color: rgb(43,69,79);">' to create one.</span><br>
<span style="color: rgb(237,230,203);">Events         : </span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(43,69,79);"> Type '</span><span style="color: rgb(232,178,111);">EVENT</span><span style="color: rgb(43,69,79);">' to create one.</span><br>
<br>
<span style="color: rgb(237,230,203);">Exits:</span><br>
<span style="color: rgb(245,216,147);">  Dir   To Vnum    Room Desc      Key     Reset Flags       Kwds</span><br>
<span style="color: rgb(182,131,76);">======= ======= =============== ======= =============== ============</span><br>
 </div>

Be sure to check `COMMANDS` to see what options are available.

The intention is for this to be the first room a player sees when they create an elf character. Therefore, we should think carefully about the naming and prose used in this room. For now, I will use placeholder text.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">To be honest, I was never a good builder. In my Imp days of ages past, I was a <i>coding</i> Imp. I needed an army of builders and admins to do the other work for me (like, actually <i>running</i> and <i>creating</i> the MUD).</p></div>

Note that, by creating the room, we actually teleported to it. This is useful so we can perform actions like `LOOK` and whatnot. OLC is very much "in-game". If you check the log, you'll see that a brand new instance of the area was spun up just for you. That's a function of it being "multi-instance"; new instances are created on demand as players enter them.

I choose to give it a narrative title, rather than a descriptive one:

```
name The Awakening
```

Even though this is a placeholder, I want to at least put _something_ for the description, using `DESC`:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(182,131,76);">-========- </span><span style="color: rgb(112,77,43);">Entering EDIT Mode </span><span style="color: rgb(182,131,76);">-=========-</span><br>
<span style="color: rgb(43,69,79);">    Type .h on a new line for help</span><br>
<span style="color: rgb(43,69,79);"> Terminate with a @ on a blank line.</span><br>
<span style="color: rgb(182,131,76);">-=======================================-</span><br>
<span style="color: rgb(237,230,203);">&gt; </span><br>
<br>
 </div>

Typing `.h` for help yields a list of really helpul commands. In particular, `.f` is your friend.

I will enter some nice, overwrought, pretentious prose:

```
    You stand next to the great old trunk of Cuivealda. The light is dim, as the dense, high canopy that forms Tauremar covers the sky. There is little ground cover, and the forest floor is a tranquil place of perpetual twilight.
```

The initial four spaces tell the formatter that this is a "book style" paragraph,
and the indentation should be preserved. There is no double-line gap between this and the preceding paragraph, either.

Use `@` on a bare line to finish the description.

The last thing to do before making it a valid "starting" area is setting room flags. Type `FLAGS` to see a list of available options.

The two flags I want:

```
flags newbies_only no_recall
```

The latter  is because I don't want people just recalling out of the start zone. There should be a narrative flow to the game, and skipping the starting area breaks that.

Now I want to hook this room up as the starting area for elves. I'll tie off this room for now with:

```
done
asave changed
```

## Setting racial starting location

The command for editing races is `RAEDIT`. Unfortunately, there's no help for that command, yet (it wasn't a part of the orginal OLC). I'll put that on my bucket list.

For now, let's just dive in:

```
raedit elf
```

This gives us this:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Name        : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">elf</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">PC race?    : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(49,78,63);">YES</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Act         : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Aff         : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">infrared</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Off         : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Imm         : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Res         : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">charm</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Vuln        : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">iron</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Form        : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">edible sentient biped mammal</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Parts       : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">head arms legs heart brains guts hands feet fingers ear eye</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Points      : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">5</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Size        : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">small</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Start Loc   : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">12000</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);">The Awakening </span><br>
<span style="color: rgb(245,216,147);">    Class      XPmult  XP/lvl(pts)   Start Loc</span><br>
<span style="color: rgb(237,230,203);">    Mage        </span><span style="color: rgb(112,77,43);">100     1000</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);"> 40</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);">    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    0</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);"> </span><br>
<span style="color: rgb(237,230,203);">    Cleric      </span><span style="color: rgb(112,77,43);">125     1000</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);"> 40</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);">    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    0</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);"> </span><br>
<span style="color: rgb(237,230,203);">    Thief       </span><span style="color: rgb(112,77,43);">100     1000</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);"> 40</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);">    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    0</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);"> </span><br>
<span style="color: rgb(237,230,203);">    Warrior     </span><span style="color: rgb(112,77,43);">120     1000</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);"> 40</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);">    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    0</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);"> </span><br>
<br>
<span style="color: rgb(237,230,203);">Str:</span><span style="color: rgb(112,77,43);">12</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">16</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);"> Int:</span><span style="color: rgb(112,77,43);">14</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">20</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);"> Wis:</span><span style="color: rgb(112,77,43);">13</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">18</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);"> Dex:</span><span style="color: rgb(112,77,43);">15</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">21</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);"> Con:</span><span style="color: rgb(112,77,43);">11</span><span style="color: rgb(64,35,30);">(</span><span style="color: rgb(112,77,43);">15</span><span style="color: rgb(64,35,30);">)</span><span style="color: rgb(237,230,203);"> </span><br>
<span style="color: rgb(237,230,203);"> 0. </span><span style="color: rgb(112,77,43);">sneak</span><br>
<span style="color: rgb(237,230,203);"> 1. </span><span style="color: rgb(112,77,43);">hide</span><br>
 </div>

By default, Mud98 allows for starting race and class locations. Otherwise, under `Start Loc`, it would say "(not used)". If you see this message, however you can enable it by openning up `mud98.cfg` in the root directory and changing this setting (uncommenting it, if needed):

```
start_loc_by_race = yes
```

Note that this is a completely safe flag; it has no effect on races that don't have a start location set, yet.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">You may have noticed that there is also a class <code>start_loc</code> and a per-race-class-combo <code>start_loc</code>. These can be used with each other freely. You can have a single spawn point for mages, a single spawn point for elves, and separate spawn points for all the other race/class combos. It's meant to give you maximum flexibility in world-building. To take advantage of this, you need to also enable <code>start_loc_by_class</code> in <code>mud98.cfg</code>.</p></div>

Restart Mud98 and log back in with your Imp character. Then `raedit elf` again. Note that now `Start Loc` is `0`, but the warning that it won't be used is gone.

I can now set the start location for elves:

```
start_loc 12000
```

Now `SHOW` has our new room as `Start Loc`:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Start Loc   : </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">12000</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);">The Awakening </span><br>
 </div>

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

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128);">The Awakening</span><br>
<span style="color: rgb(192,192,192);">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(192,192,192);">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(192,192,192);">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(192,192,192);">twilight.</span><br>
<span style="color: rgb(192,192,192);">You have no unread notes.</span><br>
<br>
<span style="color: rgb(0,128,128);">&lt;20hp 100m 100mv&gt; </span><br></div>

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">The difference in coloration is due to the fact that this character, being brand-new, is using the default, "Lope ColoUr" theme.</p></div>

If you left your Imp character logged in at this room, the new character will not see him; a quick glance at the log will tell you that a brand new instance of "Faladrin Forest" was spun up for "Newelfguy", and ne'er the twain shall meet.

## Creating a greeting for new characters

New characters all receive a generic banner that is defined in helps, but I also want a separate banner specifically for new characters who drop into the game in this room.

This brings us to two powerful new concepts novel to Mud98: **Lox scripting** and **events**.

**Lox** is a flexible, _fast_, byte-code interpreted language grafted into Mud98. It's adapted from Robert Nystrum's book, "Crafting Interpreters" (it's seriously cool; [you can buy it or read it online here](https://craftinginterpreters.com/)). Lox is a first-class citizen in Mud98, and can be used far beyond just the event system, including writing new skills and spells via OLC.

**Events** are the Mud98 replacement for MobProgs. For every MobProg trigger, there is an equivalent Mobile event trigger. Mud98 events, however, can also be attached to _Objects_ and _Rooms_, as well.

To display the message we want, we will need to create a Lox script, and call it when a new character gets dumped into this room.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">In the legacy version of this document, I used MobProgs on an invisible "triggerbot" to do this. My dissatisfaction with that solution is what prompted me to hide in whole for two years and create the current solution</p></div>

### Creating a Lox script

Let's get back into our room editor:

```
redit 12000
```

We should see this:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Description: </span><br>
<span style="color: rgb(43,69,79);">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(43,69,79);">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(43,69,79);">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(43,69,79);">twilight.</span><br>
<span style="color: rgb(237,230,203);">Room           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(245,216,147);">The Awakening</span><br>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        48</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Sector         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    inside</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Room Flags     : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">no_recall newbies_only</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Heal Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Mana Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Characters     : </span><span style="color: rgb(112,77,43);">halivar </span><br>
<span style="color: rgb(237,230,203);">Objects        : </span><span style="color: rgb(112,77,43);">(none)</span><br>
<span style="color: rgb(237,230,203);">Lox Class      : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Type '</span><span style="color: rgb(232,178,111);">LOX</span><span style="color: rgb(43,69,79);">' to create one.</span><br>
<span style="color: rgb(237,230,203);">Events         : </span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(43,69,79);"> Type '</span><span style="color: rgb(232,178,111);">EVENT</span><span style="color: rgb(43,69,79);">' to create one.</span><br>
<br>
<span style="color: rgb(237,230,203);">Exits:</span><br>
<span style="color: rgb(245,216,147);">  Dir   To Vnum    Room Desc      Key     Reset Flags       Kwds</span><br>
<span style="color: rgb(182,131,76);">======= ======= =============== ======= =============== ============</span><br></div>

The important fields for this task are `Lox Class` and `Events`. To start, let's create an `on_login` callback for the login event we will eventually create:

```
lox
```

You'll be thrown into a string editor that is pretty close to what we saw for the room description editor, but there are some key differences. If you type `.h`, you'll get this help:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(232,178,111);">Lox Script help (commands on blank line):   </span><br>
<span style="color: rgb(112,77,43);">.r 'old' 'new'   </span><span style="color: rgb(232,178,111);">- replace a substring </span><br>
<span style="color: rgb(112,77,43);">                 </span><span style="color: rgb(232,178,111);">  (requires '', "") </span><br>
<span style="color: rgb(112,77,43);">.h               </span><span style="color: rgb(232,178,111);">- get help (this info)</span><br>
<span style="color: rgb(112,77,43);">.s               </span><span style="color: rgb(232,178,111);">- show script so far  </span><br>
<span style="color: rgb(112,77,43);">.clear           </span><span style="color: rgb(232,178,111);">- clear script so far </span><br>
<span style="color: rgb(112,77,43);">.v               </span><span style="color: rgb(232,178,111);">- validate byte-code compilation</span><br>
<span style="color: rgb(112,77,43);">.ld &lt;num&gt;        </span><span style="color: rgb(232,178,111);">- delete line &lt;num&gt;</span><br>
<span style="color: rgb(112,77,43);">.li &lt;num&gt; &lt;txt&gt;  </span><span style="color: rgb(232,178,111);">- insert &lt;txt&gt; on line &lt;num&gt;</span><br>
<span style="color: rgb(112,77,43);">.lr &lt;num&gt; &lt;txt&gt;  </span><span style="color: rgb(232,178,111);">- replace line &lt;num&gt; with &lt;txt&gt;</span><br>
<span style="color: rgb(112,77,43);">.x               </span><span style="color: rgb(232,178,111);">- cancel changes</span><br>
<span style="color: rgb(112,77,43);">@                </span><span style="color: rgb(232,178,111);">- compile and save script</span><br></div>

There's no formatter, at the moment, because my notions of code style are opinionated, and I don't want to force that on you. Here are some important new commands not present in the "string" editor:

* `.x` - **Cancel changes and exit the script editor.** This is important because it restores the original script (or lack thereof). Use this when your changes just aren't working out.
* `.v` - **Validate byte-code compilation.** Use this to verify that your script compiles to byte-code without leaving the editor. This doesn't save the script to the Entity being edited, however, so it's safe to run. If there are any errors in your Lox script, they will display on your screen.
* `@` - **Compile and save script.** Unlike `.v`, this command commits your changes to the prototype of the Entity being edited. New instances of that Entity will have the compiled byte-code of the script attached to them. If the script does not yet compile correctly, this will save the script to the Entity prototype and allow you to edit it later; however, no byte code will be attached to instances of the Entity, themselves.

For now, let's go ahead and enter our script:

```
// This function gets called the first time a character logs in as an elf, 
// after character creation.
on_login(vch) {
    // Only newly-created characters
    if (vch.level > 1 or vch.race != Race.Elf)
        return;
        
    vch.send(
        "^*You step off the end of an old rope ladder at the base of "
        "Cuivealda, the Tree of Awakening. Your years in the nurturing "
        "hands of the Tetyayath have come to an end.^/^/"
        "Now you must go out into the world and find your destiny...^/^x")
}
```

If you type `.s`, you will see that our built-in editor also has _syntax-highlighting_:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Script so far:</span><br>
<span style="color: rgb(112,77,43);"> 1</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(79,93,66);">// This function gets called the first time a character logs in as an elf, </span><br>
<span style="color: rgb(112,77,43);"> 2</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(79,93,66);">// after character creation.</span><br>
<span style="color: rgb(112,77,43);"> 3</span><span style="color: rgb(237,230,203);">. on_login</span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">) {</span><br>
<span style="color: rgb(112,77,43);"> 4</span><span style="color: rgb(237,230,203);">.     </span><span style="color: rgb(79,93,66);">// Only newly-created characters</span><br>
<span style="color: rgb(112,77,43);"> 5</span><span style="color: rgb(237,230,203);">.     </span><span style="color: rgb(43,69,79);">if </span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">level </span><span style="color: rgb(112,77,43);">&gt; </span><span style="color: rgb(62,112,142);">1 </span><span style="color: rgb(43,69,79);">or </span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">race </span><span style="color: rgb(112,77,43);">!= </span><span style="color: rgb(237,230,203);">Race</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">Elf</span><span style="color: rgb(112,77,43);">)</span><br>
<span style="color: rgb(112,77,43);"> 6</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(43,69,79);">return</span><span style="color: rgb(112,77,43);">;</span><br>
<span style="color: rgb(112,77,43);"> 7</span><span style="color: rgb(237,230,203);">.  </span><br>
<span style="color: rgb(112,77,43);"> 8</span><span style="color: rgb(237,230,203);">.     vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">send</span><span style="color: rgb(112,77,43);">(</span><br>
<span style="color: rgb(112,77,43);"> 9</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"^*You step off the end of an old rope ladder at the base of "</span><br>
<span style="color: rgb(112,77,43);">10</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"Cuivealda, the Tree of Awakening. Your years in the nurturing "</span><br>
<span style="color: rgb(112,77,43);">11</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"hands of the Tetyayath have come to an end.^/^/"</span><br>
<span style="color: rgb(112,77,43);">12</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"Now you must go out into the world and find your destiny...^/^x"</span><span style="color: rgb(112,77,43);">)</span><br>
<span style="color: rgb(112,77,43);">23</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(112,77,43);">}</span><br>
<span style="color: rgb(112,77,43);">24</span><span style="color: rgb(237,230,203);">. </span><br></div>

Note that the colors used by the syntax highlighter can be set in your theme (via the `THEME` command).

Let's validate our script using `.v`:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(29,50,48);">***</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(232,178,111);">Class "room_12000" for room 12000 compiled successfully.</span><br></div>

As the message indicates, what we're _really_ doing is specifying the contents of a class dedicated to Room VNUM 12000 called, appropriately enough, `room_12000`. That's why we don't declare `on_login` as a `func`: class methods in Lox don't use that keyword; just the method name and its arguments are sufficient.

Since it completed successfully, we can call it done and attach it to the object with `@`:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(29,50,48);">***</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(232,178,111);">Class "room_12000" for room 12000 compiled successfully and assigned.</span><br></div>

I'll go over the script in a moment. First, let's protect what we've got so far:

```
asave changed
```

### Anatomy of an event callback

Our script defines what's called a "callback", a function (or in this case, a class member) that is referred to by an outside actor when needed. To do that, we will need to register this callback by creating an event that references it. We will do that in the next step.

First, we have a comment:

```
// This function gets called the first time a character logs in as an elf, 
// after character creation.
```

A **comment** is in-code documentation that is not compiled, and does not get translated to byte-code. Use comments to make clear what you are trying to do, and what the purpose of your code is. 

```
on_login(vch) {
}
```

This is **class method**, named `on_login`. Class methods are normally defined in a `class {..}` block, but Entity-based Lox scripts are treated as class bodies with bespoke, predetermined names.

The `on_login` method has a single argument, `vch`. If you are at all familiar with ROM code, you'll know that this is shorthand for "victim character", and tell us that the target of the callback is passed in as an argument. In this case, `vch` will be the character logging in.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">And what of the room, itself? Why is there no <code>room</code> argument? Because this method will already a reference to it's owning Entity. We'll see that in action when we add a Mobile-based event.</p></div>

Now we check to see if it's appropriate to send a message to the character:

```
    // Only newly-created characters
    if (vch.level > 1 or vch.race != Race.Elf)
        return;
```

If the character is above level 1 (not a newbie) or not an elf (as this message is for starting elves), we bail. No message is sent to the player.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;">Note the semicolon (<code>;</code>) after <code>return</code>. This is one of the few areas where I deviate from standard Lox, which has no EOL semicolons. In Mud98's incarnation of Lox, semicolons are optional <i>except</i> bare <code>return</code>s that don't actually return a value. In that case, they are required. This is a technical issue in Mud98's Lox compiler that I may (or may not) fix.</p></div>

Next we send an actual message to the user:

```
    vch.send(
        "^*You step off the end of an old rope ladder at the base of "
        "Cuivealda, the Tree of Awakening. Your years in the nurturing "
        "hands of the Tetyayath have come to an end.^/^/"
        "Now you must go out into the world and find your destiny...^/^x")
```

So that's our first Lox script. Let's wire it up to a Login event and see how it does.

### Wiring up the Login event

While editing room 12000, let's look at what's available to us with events:

```
event
```

We'll get a syntax helper like so:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(232,178,111);">Syntax: event set &lt;trigger&gt; [&lt;callback&gt; [&lt;criteria&gt;]]</span><br>
<span style="color: rgb(232,178,111);">        event delete &lt;trigger&gt;</span><br></div>

The only _required_ parameter is the trigger. Every trigger has a default callback, and _usually_ has a default criteria value if one is needed (which the Login event doesn't). Because I knew ahead of time what the default was, I don't need it:

```
event set login
```

You should get an "Event created" message, which means we're good to go. Verify it by pressing `Enter` while in `REDIT` mode:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Description: </span><br>
<span style="color: rgb(43,69,79);">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(43,69,79);">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(43,69,79);">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(43,69,79);">twilight.</span><br>
<span style="color: rgb(237,230,203);">Room           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(245,216,147);">The Awakening</span><br>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">        48</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Sector         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    inside</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Room Flags     : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">no_recall newbies_only</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Heal Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Mana Recover   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Characters     : </span><span style="color: rgb(112,77,43);">halivar </span><br>
<span style="color: rgb(237,230,203);">Objects        : </span><span style="color: rgb(112,77,43);">(none)</span><br>
<span style="color: rgb(237,230,203);">Lox Class      : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">room_12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Type '</span><span style="color: rgb(232,178,111);">LOX</span><span style="color: rgb(43,69,79);">' to edit.</span><br>
<br>
<span style="color: rgb(237,230,203);">Events:</span><br>
<span style="color: rgb(245,216,147);"> Trigger          Callback (Args)     Criteria</span><br>
<span style="color: rgb(182,131,76);">========= ================ ========== ========</span><br>
<span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  Login</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(43,69,79);">         on_login (mob)      </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">      </span><span style="color: rgb(64,35,30);">]</span><br>
<br>
<span style="color: rgb(237,230,203);">Exits:</span><br>
<span style="color: rgb(245,216,147);">  Dir   To Vnum    Room Desc      Key     Reset Flags       Kwds</span><br>
<span style="color: rgb(182,131,76);">======= ======= =============== ======= =============== ============</span><br></div>

Now you see "Events" populated with our trigger, as well as a reference to our callback. When you add an event, the "Args" column will tell you what the event trigger will attempt to pass as arguments to the trigger. If you don't follow that format, the event won't work.

Before we test it out, let's save our work:

```
asave changed
```

Now create a new elf character, and you should see this upon being dumped in the starting room:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128);">You step off the end of an old rope ladder at the base of Cuivealda, the Tree of</span><br>
<span style="color: rgb(0,128,128);">Awakening. Your years in the nurturing hands of the Tetyayath have come to an </span><br>
<span style="color: rgb(0,128,128);">end.</span><br>
<br>
<span style="color: rgb(0,128,128);">Now you must go out into the world and find your destiny...</span><br>
<br>
<span style="color: rgb(0,128,128);">The Awakening</span><br>
<span style="color: rgb(192,192,192);">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(192,192,192);">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(192,192,192);">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(192,192,192);">twilight.</span><br>
<span style="color: rgb(192,192,192);">You have no unread notes.</span><br>
<br>
<span style="color: rgb(0,128,128);">&lt;20hp 100m 100mv&gt; </span><br>
 </div>

It works! You just wrote your first Lox script, and your first event. What's cool about the Login event is that it triggers _before_ you are dumped into the starting room, so it lets us write a neat little prologue to establish a narrative for the new character.

But now I need to establish next steps for the character.

### Delayed scripts

I like for calls-to-action for the player to follow be the last thing they before the prompt (like how tutorials work [have you checked out `TUTORIAL`, yet?]). But everything we do inside the Login event happens before we even show them a room description.

The answer is modify our Login event to kick off a delayed message. Just 1 tick is enough to guarantee is displays after the room description.

First, let's edit our Lox script again:

```
redit 12000 (if you need to)
lox
```

Now we need to insert some lines using the `.li` command:

```
.li 13
.li 14    delay(1, () -> {
.li 15        vch.send("^jYou have instructions to meet your old mentor in a "
.li 16            "clearing just to the east. There you will continue your "
.li 17            "training. Type '^*EXITS^j' to see what lies in that "
.li 18            "direction. Type '^*EAST^j' to go there.^x")
.li 19    })
```

The Lox function `delay` takes two parameters:
1. The number to ticks to delay by; in this case, 1.
2. An invokable function or method to call at the end of the delay.

In this case, we pass what's called a _lamda_ as the callable. It's basically an ad hoc, unnamed function.

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p style="margin: 0;"> Lamdas can have arguments, but this one does not. Everything it needs is in its own scope. By simply mentioning <code>vch</code>, it "captures" that variable and keeps it from getting garbage-collected until it gets a chance to run.</p></div>

After one tick, we send the given message to the player that triggered the event.

When you're done, `.s` should look like this:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Script so far:</span><br>
<span style="color: rgb(112,77,43);"> 1</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(79,93,66);">// This function gets called the first time a character logs in as an elf, </span><br>
<span style="color: rgb(112,77,43);"> 2</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(79,93,66);">// after character creation.</span><br>
<span style="color: rgb(112,77,43);"> 3</span><span style="color: rgb(237,230,203);">. on_login</span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">) {</span><br>
<span style="color: rgb(112,77,43);"> 4</span><span style="color: rgb(237,230,203);">.     </span><span style="color: rgb(79,93,66);">// Only newly-created characters</span><br>
<span style="color: rgb(112,77,43);"> 5</span><span style="color: rgb(237,230,203);">.     </span><span style="color: rgb(43,69,79);">if </span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">level </span><span style="color: rgb(112,77,43);">&gt; </span><span style="color: rgb(62,112,142);">1 </span><span style="color: rgb(43,69,79);">or </span><span style="color: rgb(237,230,203);">vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">race </span><span style="color: rgb(112,77,43);">!= </span><span style="color: rgb(237,230,203);">Race</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">Elf</span><span style="color: rgb(112,77,43);">)</span><br>
<span style="color: rgb(112,77,43);"> 6</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(43,69,79);">return</span><span style="color: rgb(112,77,43);">;</span><br>
<span style="color: rgb(112,77,43);"> 7</span><span style="color: rgb(237,230,203);">.  </span><br>
<span style="color: rgb(112,77,43);"> 8</span><span style="color: rgb(237,230,203);">.     vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">send</span><span style="color: rgb(112,77,43);">(</span><br>
<span style="color: rgb(112,77,43);"> 9</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"^*You step off the end of an old rope ladder at the base of "</span><br>
<span style="color: rgb(112,77,43);">10</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"Cuivealda, the Tree of Awakening. Your years in the nurturing "</span><br>
<span style="color: rgb(112,77,43);">11</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"hands of the Tetyayath have come to an end.^/^/"</span><br>
<span style="color: rgb(112,77,43);">12</span><span style="color: rgb(237,230,203);">.         </span><span style="color: rgb(245,216,147);">"Now you must go out into the world and find your destiny...^/^x"</span><span style="color: rgb(112,77,43);">)</span><br>
<span style="color: rgb(112,77,43);">13</span><span style="color: rgb(237,230,203);">. </span><br>
<span style="color: rgb(112,77,43);">14</span><span style="color: rgb(237,230,203);">.     delay</span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(62,112,142);">1</span><span style="color: rgb(112,77,43);">, () -&gt; {</span><br>
<span style="color: rgb(112,77,43);">15</span><span style="color: rgb(237,230,203);">.         vch</span><span style="color: rgb(112,77,43);">.</span><span style="color: rgb(237,230,203);">send</span><span style="color: rgb(112,77,43);">(</span><span style="color: rgb(245,216,147);">"^jYou have instructions to meet your old mentor in a "</span><br>
<span style="color: rgb(112,77,43);">16</span><span style="color: rgb(237,230,203);">.             </span><span style="color: rgb(245,216,147);">"clearing just to the east. There you will continue your "</span><br>
<span style="color: rgb(112,77,43);">17</span><span style="color: rgb(237,230,203);">.             </span><span style="color: rgb(245,216,147);">"training. Type '^*EXITS^j' to see what lies in that "</span><br>
<span style="color: rgb(112,77,43);">18</span><span style="color: rgb(237,230,203);">.             </span><span style="color: rgb(245,216,147);">"direction. Type '^*EAST^j' to go there.^x"</span><span style="color: rgb(112,77,43);">)</span><br>
<span style="color: rgb(112,77,43);">19</span><span style="color: rgb(237,230,203);">.     </span><span style="color: rgb(112,77,43);">})</span><br>
<span style="color: rgb(112,77,43);">20</span><span style="color: rgb(237,230,203);">. </span><span style="color: rgb(112,77,43);">}</span><br>
<span style="color: rgb(112,77,43);">21</span><span style="color: rgb(237,230,203);">. </span><br></div>

Now we tie a bow around it for testing:

```
@
asave area
```

Then we test it again with a new character:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128);">You step off the end of an old rope ladder at the base of Cuivealda, the Tree of</span><br>
<span style="color: rgb(0,128,128);">Awakening. Your years in the nurturing hands of the Tetyayath have come to an </span><br>
<span style="color: rgb(0,128,128);">end.</span><br>
<br>
<span style="color: rgb(0,128,128);">Now you must go out into the world and find your destiny...</span><br>
<br>
<span style="color: rgb(0,128,128);">The Awakening</span><br>
<span style="color: rgb(192,192,192);">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(192,192,192);">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(192,192,192);">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(192,192,192);">twilight.</span><br>
<span style="color: rgb(192,192,192);">You have no unread notes.</span><br>
<span style="color: rgb(128,128,0);">You have instructions to meet your old mentor in a clearing just to the east.</span><br>
<span style="color: rgb(128,128,0);">There you will continue your training. Type '</span><span style="color: rgb(0,128,128);">EXITS</span><span style="color: rgb(128,128,0);">' to see what lies in that </span><br>
<span style="color: rgb(128,128,0);">direction. Type '</span><span style="color: rgb(0,128,128);">EAST</span><span style="color: rgb(128,128,0);">' to go there.</span><br>
<br>
<span style="color: rgb(0,128,128);">&lt;20hp 100m 100mv&gt; </span><br></div>

What's great about this is that it's not a part of the room description, which I want to always keep as IC ("in character in ye olde MUD speak) as possible.

Now that we've told the player to go east, we probably need to start building our area out.

## Room building

In creating our first login event, I sort of put the cart before the horse. My first checklist item is actually to build out the area. Here is the map I came up with, with VNUMs listed for the rooms:

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
      START                     |
    +-------+   +-------+   +---+---+
    | 12000 |---| 12002 |---| 12005 |
    +-------+   +-------+   +-------+
```

<div style="background-color: rgb(32,64,255,0.1);padding: 1em;border-radius:5px;margin-bottom:1em;"><p>IMXP, the best way to build an area is to sketch it out first, assigning VNUMs ahead of time. I space VNUM's by 5, but if rooms will be densely packed with mobs and objs, I space by 10. This lets me group obj and mob VNUMs by room in a way that is predictable and easy to manage.</p>
<p style="margin: 0">Sketch out the area as uniform blocks. Even in my MUD days (late 90's) auto-mappers were ubiquitious, and players were more partial to area layouts that didn't muck with their maps.</p></div>

To demonstrate the two different ways to establish room connections, I'm going to accidentally forget to create room 12002. To instantly buid out rooms, use `DIG` while editing room 12000:

```
east dig 12002
```

This created a room with VNUM 12002, and moved you to it. Furthermore, it put you into `REDIT` mode in this new room. Using this, you can chain your commands to quickly sketch out the area.

I also make use of the `LINK` command to add two-way connections between existing rooms. This doesn't move you, however, so I do have one manual move (note that this, unlike traditional movement, cannot be abbreviated; to move west, you must type `west` and not `w`).

```
east dig 12005
north dig 12010
west dig 12011
north dig 12012
north dig 12013
east dig 12014
east dig 12015
south dig 12016
south dig 12017
west link 12010
west
north dig 12018
west link 12012
north link 12014
east link 12016
asave changed
```

And with that, our rooms are built out and ready to go (albeit without any descriptions, or anything to even indicate that you have switched rooms).

## Quest-driven expansion

Our little framing story tells the player to seek out an "old mentor" to the east. I need Room 12002 for another purpose, so I'll put him in Room 12005. If you'll recall, one of our story beats for this area is about "Findorian the Forest Speaker".

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
faction Tauremar
short Findorian, the Woodspeaker

long Findorian, the Woodspeaker, kneels in the grassy clearing, gazing intently at
the dark woods surrounding you.
```

His stats are pretty bland. You can try to figure out what his hitroll and dice, and all that extra stuff should be, or you can set them to sensible defaults with:

```
recval
```

I don't know what that means. But it uses a lookup table in `tables.c` to set default values that the entire MUD, for the most part, adheres to. You can tweak to your preference.

If you want to see these combat ratings on a sample of other mobs in the world, you can use the `MOBLIST` command (not to be confused with `MLIST`, which is only in OLC and is more bare-bones).

Once all the above is done, he should look like this (hit `Enter` in `MEDIT`):

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Name:        </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">findorian forestspeaker</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Vnum:        </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 12005</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Area:        </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    48</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Faction:     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 12000</span><span style="color: rgb(64,35,30);">] </span><span style="color: rgb(43,69,79);">Tauremar</span><br>
<span style="color: rgb(237,230,203);">Level:       </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    20</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Sex:     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  none</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Group:   </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    0</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Align:       </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">     0</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Hitroll: </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">     0</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Dam type: </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">none</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Hit dice:    </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  3d9  + 333</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Damage dice:  </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">  2d8  +   5</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Mana dice:   </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);"> 20d12 + 100</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Material:     </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">     unknown</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Race:        </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">         elf</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Size:         </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">      medium</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Start pos.:  </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    standing</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> Default pos.: </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">    standing</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Wealth:      </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">      0 cp</span><span style="color: rgb(64,35,30);">]</span><span style="color: rgb(237,230,203);"> </span><span style="color: rgb(43,69,79);">no coins</span><br>
<span style="color: rgb(237,230,203);">Armor:       </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(43,69,79);">pierce: </span><span style="color: rgb(112,77,43);">-40</span><span style="color: rgb(43,69,79);">  bash: </span><span style="color: rgb(112,77,43);">-40</span><span style="color: rgb(43,69,79);">  slash: </span><span style="color: rgb(112,77,43);">-40</span><span style="color: rgb(43,69,79);">  magic: </span><span style="color: rgb(112,77,43);">-30</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Affected by: </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">infrared</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">Act:         </span><span style="color: rgb(64,35,30);">[</span><span style="color: rgb(112,77,43);">npc sentinel</span><span style="color: rgb(64,35,30);">]</span><br>
<span style="color: rgb(237,230,203);">       Form: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">edible sentient biped mammal</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">      Parts: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">head arms legs heart brains guts hands feet fingers ear eye</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">        Imm: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">        Res: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">charm weapon bash</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">       Vuln: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">      iron</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">        Off: </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">assist_race</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Short descr: </span><span style="color: rgb(43,69,79);">Findorian, the Forestspeaker</span><br>
<span style="color: rgb(237,230,203);">Long descr:</span>
<span style="color: rgb(43,69,79);"> Findorian, the Forestspeaker, kneels in the grassy clearing, gazing intently</span><br>
<span style="color: rgb(43,69,79);">at the dark woods surrounding you.</span><br><span style="color: rgb(237,230,203);">Description:</span><br>
<span style="color: rgb(237,230,203);">Lox Class      : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Type '</span><span style="color: rgb(232,178,111);">LOX</span><span style="color: rgb(43,69,79);">' to create one.</span><br>
<span style="color: rgb(237,230,203);">Events         : </span><span style="color: rgb(112,77,43);">(none)</span><span style="color: rgb(43,69,79);"> Type '</span><span style="color: rgb(232,178,111);">EVENT</span><span style="color: rgb(43,69,79);">' to create one.</span><br></div>

Once you have the mob where you want him, you can set him in place:

```
done
redit 12005
mreset 12005 -1 1
done
asave changed
```

Note that an instance of Findorian is immediately created in the room with you.

### Creating the player's first quest

The first quest the elf player will receive is to go see Findorian, who will dispense the next set of quests. Since our first room will initiate the first quest, I'll use that VNUM:

```
qedit create 12000
```

This creates a blank slate of a quest:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(232,178,111);">New quest created.</span><br>
<span style="color: rgb(237,230,203);">Quest          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">(none)</span><br>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(112,77,43);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Type           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);"> visit_mob</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Level          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         1</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">End            : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         0</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">(none)</span><br>
<span style="color: rgb(237,230,203);">Target         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         0</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">(none)</span><br>
<span style="color: rgb(237,230,203);">XP             : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         0</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Currency       : </span><span style="color: rgb(112,77,43);">(none)</span><br>
<span style="color: rgb(237,230,203);">Faction        : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><br>
<span style="color: rgb(237,230,203);">Reputation     : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         0</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Reward Items   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><br>
<span style="color: rgb(237,230,203);">Entry          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ]</span><br></div>

I then fill in some information:

```
name An Old Friend
level 1
end 12005
target 12005
xp 100
faction 12000
rewardrep 3000
rewardcopper 10
```

I also add text for the quest log entry with the `ENTRY` command. Here;s what it looks like when finished:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(17,18,21); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(237,230,203);">Quest          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">An Old Friend</span><br>
<span style="color: rgb(237,230,203);">Area           : </span><span style="color: rgb(112,77,43);">Faladrin Forest</span><br>
<span style="color: rgb(237,230,203);">Type           : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);"> visit_mob</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Level          : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">         1</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">End            : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12005</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Findorian, the Forestspeaker</span><br>
<span style="color: rgb(237,230,203);">Target         : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12005</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Findorian, the Forestspeaker</span><br>
<span style="color: rgb(237,230,203);">XP             : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">       100</span><span style="color: rgb(64,35,30);"> ]</span><br>
<span style="color: rgb(237,230,203);">Currency       : </span><span style="color: rgb(112,77,43);">10 copper</span><br>
<span style="color: rgb(237,230,203);">Faction        : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">     12000</span><span style="color: rgb(64,35,30);"> ] </span><span style="color: rgb(43,69,79);">Tauremar</span><br>
<span style="color: rgb(237,230,203);">Reward Items   : </span><span style="color: rgb(64,35,30);">[ </span><span style="color: rgb(112,77,43);">    (none)</span><span style="color: rgb(64,35,30);"> ] </span><br>
<span style="color: rgb(237,230,203);">Entry: </span><br>
<span style="color: rgb(43,69,79);">Before you step out into the greater world, you must be trained.  To that</span><br>
<span style="color: rgb(43,69,79);">end, your old mentor Findorian has has taken the responsibility to ensure</span><br>
<span style="color: rgb(43,69,79);">you are properly made ready.  He waits for you just to the east of</span><br>
<span style="color: rgb(43,69,79);">Cuivealda.</span><br></div>

The default type is `visit_mob`, which is just right for us right now. I need to go back and edit the Login event on Room 12000. This is the new, updated code:

```
// This function gets called the first time a character logs in as an elf, 
// after character creation.
on_login(vch) {
    // Only newly-created characters
    if (vch.level > 1 or vch.race != Race.Elf)
        return;
        
    vch.send(
        "^*You step off the end of an old rope ladder at the base of "
        "Cuivealda, the Tree of Awakening. Your years in the nurturing "
        "hands of the Tetyayath have come to an end.^/^/"
        "Now you must go out into the world and find your destiny...^/^x")

    if (vch.can_quest(12000)) {
        delay(1, () -> {
            vch.send("^jYou have instructions to meet your old mentor in a "
                "clearing just to the east. There you will continue your "
                "training. Type '^*EXITS^j' to see what lies in that "
                "direction. Type '^*EAST^j' to go there.^x")
            vch.grant_quest(12000)
            vch.send("^jType '^*QUEST^j' to view your quest log.^x")
        })
    }
}
```

The quest system in Mud98 is a novel addition of my own. There is an existing quest system for ROM that was fairly popular back in the day, but it relies on hard-coded quests and had no OLC support. Mud98's quest system is entirely OLC-driven.

Here are the new commands used in this script, and what they do:
- `vch.can_quest(quest_vnum)` returns `true` if the player (in `vch`) is eligible to receive the quest. If they already on the quest, or have completed it, it returns false. Effectively, this makes this text only display once.
- `vch.grant_quest(quest_vnum)` adds the quest to their quest log.

After compiling the script and trying it with a new characters, this is what the player sees:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">You step off the end of an old rope ladder at the base of Cuivealda, the Tree of</span><br>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">Awakening. Your years in the nurturing hands of the Tetyayath have come to an </span><br>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">end.</span><br>
<br>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">Now you must go out into the world and find your destiny...</span><br>
<br>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">The Awakening</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">    You stand next to the great old trunk of Cuivealda.  The light is</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">dim, as the dense, high canopy that forms Tauremar covers the sky.  There is</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">little ground cover, and the forest floor is a tranquil place of perpetual</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">twilight.</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">You have no unread notes.</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">You have instructions to meet your old mentor in a clearing just to the east.</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">There you will continue your training. Type '</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">EXITS</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">' to see what lies in that </span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">direction. Type '</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">EAST</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">' to go there.</span><br>
<br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">You have started the quest, </span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">"An Old Friend</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">".</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">Type '</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">QUEST</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">' to view your quest log.</span><br></div>

Here is what they see when they type `QUEST`:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">Active Quests in Faladrin Forest:</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">1. </span><span style="color: rgb(255,255,255); background: rgb(0,0,0); ">An Old Friend </span><span style="color: rgb(0,0,255); background: rgb(0,0,0); ">[</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">Level 1</span><span style="color: rgb(0,0,255); background: rgb(0,0,0); ">] </span><br>
<span style="color: rgb(128,128,128); background: rgb(0,0,0); ">Before you step out into the greater world, you must be trained.  To that</span><br>
<span style="color: rgb(128,128,128); background: rgb(0,0,0); ">end, your old mentor Findorian has has taken the responsibility to ensure</span><br>
<span style="color: rgb(128,128,128); background: rgb(0,0,0); ">you are properly made ready.  He waits for you just to the east of</span><br>
<span style="color: rgb(128,128,128); background: rgb(0,0,0); ">Cuivealda.</span><br></div>

This list will obviously grow as the player progresses. World quests (those outside their immediate area) will be displayed as a collapsed list below. Completed quests are hidden.

### Finishing the quest

Findorian (mob VNUM `12005`) is our quest handler. We designated him both as the quest "target" and quest "end".  We need a new `on_greet` script to receive and handle the quest:

```
medit 12005
lox
```

Add this to his Lox script (to be compiled as class `mob_12005`):

```
on_greet(vch) {
    if (vch.can_finish_quest(12000)) {
        say("There you are, ${vch.name}; I'm glad to see you. I wish I could "
            "ease you into the brutal World Below, but we have no such luck.")
        echo("$n sighs.")
        say("I need your help with something, and it's not going to be "
            "pleasant.")
        vch.finish_quest(12000)
    }
}
```

I then assign this to an event on Findorian with the `GREET` trigger:

```
event set greet
asave changed
```

So, how did we do? This is what a new character sees when they enter the room with Findorian:

<div style="font-family: 'Consolas', 'Courier New', 'Monospace', 'Courier'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(0,0,0); padding:1em;  margin-bottom:1em; border-radius: 5px;"><style type='text/css'><!-- span { white-space: pre-wrap; } --></style>
<span style="color: rgb(0,128,128); background: rgb(0,0,0); ">The Training Ground</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">  Dappled sunlight breaks through the canopy in this clearing.  A patch of</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">grass grows, green and vibrant, in the center.  Dark pathways through the</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">trees go out in all directions.</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">  </span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">[</span><span style="color: rgb(255,255,0); background: rgb(0,0,0); ">?</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">]</span><span style="color: rgb(192,192,192); background: rgb(0,0,0); "> Findorian, the Forestspeaker, kneels in the grassy clearing, gazing</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">intently at the dark woods surrounding you.</span><br><span style="color: rgb(0,128,0); background: rgb(0,0,0); ">Findorian, the Forestspeaker says </span>
<span style="color: rgb(0,128,0); background: rgb(0,0,0); ">'</span><span style="color: rgb(0,255,0); background: rgb(0,0,0); ">There you are, Elithir; I'm glad to see you. I wish I could ease you into the </span><br>
<span style="color: rgb(0,255,0); background: rgb(0,0,0); ">brutal World Below, but we have no such luck.</span><span style="color: rgb(0,128,0); background: rgb(0,0,0); ">'</span><br>
<span style="color: rgb(192,192,192); background: rgb(0,0,0); ">Findorian, the Forestspeaker sighs.</span><br>
<span style="color: rgb(0,128,0); background: rgb(0,0,0); ">Findorian, the Forestspeaker says '</span><span style="color: rgb(0,255,0); background: rgb(0,0,0); ">I need your help with something, and it's not</span><br>
<span style="color: rgb(0,255,0); background: rgb(0,0,0); ">going to be pleasant.</span><span style="color: rgb(0,128,0); background: rgb(0,0,0); ">'</span><br>
<br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">You have completed the quest, "</span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">An Old Friend</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">".</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">You have been awarded </span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">100</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); "> xp.</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">You receive </span><span style="color: rgb(0,128,128); background: rgb(0,0,0); ">10 copper</span><span style="color: rgb(128,128,0); background: rgb(0,0,0); ">.</span><br>
<span style="color: rgb(128,128,0); background: rgb(0,0,0); ">Your reputation with Tauremar increases (Neutral -&gt; Friendly).</span><br></div>

The player has now completed their first quest. I could have done a more "automated" way of handling quests that didn't involve events, but I believe this method requires builders to create a more customized and narrative experience around levelling.

The `[?]` symbol in front of Findorian's name is a short-hand from MMO's meaning "this person is ready to complete your quest." In this case, the process was automated. Other quests may require some interaction. It's up to you.

## Newbie's first grind

In "MUD School", a PC needed to go kill a bunch of goblins in a pen for XP. The goblins were weak and cowering, and it was very difficult for the PC to get injured (it could happen, it was just hard). The idea was that you had a safe way to practice initiating combat without the danger of dying at level 1. It was pretty contrived, but it served its purpose.

Since the intention of this brand-new newbie area is to perform the same role, but in a more narrative fashion, I need to come up with a story and a quest for it.

So here it is: something is poisoning the forest, and a number of animals are getting sick and dying from it. The "what" is for a later quest, but the first one is conservation: humanely (elfly?) cull the sick animals. 

### The quest area

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
mreset 12010 -1 1
```

### Cloning mobs

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

### Creating a grind quest

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
Wait, I messed up; back up

That fight took _forever_. Why? Because my character had 1% in all skills and base ability scores. Also, no gear. That won't do.

## Adding a tutorial 

In MUD school, you `TRAIN` and `PRACTICE` before you fight. So I'm going to splice in some rooms between 12000 and 12005 to equip and train the player.

Here's how I create the new layout:

```
redit 12000
east delete
east dig 12002
east dig 12003
east link 12005
```

The first `delete` breaks the link between rooms `12000` and `12005`. Then I dig two new rooms east of `12000`, and finally link it with `12005`.

I started with 12002 because MobProg `12001` is already used by the triggerbot in room `12000`. As I said, I like to at least _try_ to keep my VNUMs in sync. The decision to start the second room at VNUM 12005 turns out to be wise, in retrospect, as (as I predicted) I might have needed to insert a room or two inbetween.

We can successively dig serially like this because each `dig` moves us into the new room, and sets that as our edit point.

### The training room

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
Be sure to ^*TRAIN^x your abilities and ^*PRACTICE^x your skills.
Never be afraid to seek ^*HELP^x if you lack understanding.
 
--F
```

Now, in my example, I have the config settings `train_anywhere` and `practice_anywhere` enabled. If you don't, you need to maybe need to move this part over to Findorian and add the `train` and `practice` flags to him, or add them to the triggerbot we're about to create.

To let the player know to look at the note, I want to do an OOC announcement with a triggerbot. First, the Mob Prog:

```
mpedit create 12002
code
if hasquest $n 12000
    mob echoat $n ^jTo read the note tacked to the tree, type any of the
following:^j
    mob echoat $n ^*READ NOTE^j, ^*LOOK NOTE^j, or ^*EXAMINE NOTE^j.
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

### The equipping room

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
wear body around
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
12007  0   simple hemp wristbands   [    1] [    1] [    1] [    0] [    0] 
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
[ 9] O[12007] simple hemp w inside              O[12008]  0- 1 a stout oaken c
```

Now if you log into the game with a newbie char, you can go up to the chest and perform the following commands:

```
open chest
look in chest
```

Which produces the following:

```
A stout oaken chest holds:
     simple hemp wristbands
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
    mob echoat $n ^jYour mentor has left you some gear to help you on your^x
    mob echoat $n ^jjourney. It's inside a chest by the tree.^x
    mob echoat $n ^jTry the following commands:^x
    mob echoat $n ^j- ^*OPEN CHEST^x
    mob echoat $n ^j- ^*LOOK IN CHEST^x
    mob echoat $n ^j- ^*TAKE ALL FROM CHEST^x
    mob echoat $n ^j- ^*EQUIP ALL^x
    mob echoat $n ^jYou can also look at your inventory with:^x
    mob echoat $n ^j- ^*EQ^j (short for ^*EQUIPMENT^j)^x
    mob echoat $n ^j- ^*INV^j (short for ^*INVENTORY^j)^x
    mob echoat $n ^jYou can see the effect of donning this gear by viewing your stats:^x
    mob echoat $n ^j- ^*SCORE^j (which you can abbreviate to ^*SC^j)^x
endif
@
done
medit create 12003
copy 12000
name triggerbot_12003
addprog 12003 greet 100
done
redit 12003
mreset 12003 -1 1
done
asave changed
```

Now if we load into a new character, we can see the triggerbots in action; prompting us and letting our newbie know how to proceed.

Now, with all of our equipment, we can finish testing the `kill_mob` quest.

## Completing newbie's first grind

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

Home: [Documentation](index)
