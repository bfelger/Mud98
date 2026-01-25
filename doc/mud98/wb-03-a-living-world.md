# Worldcrafting from Scratch Pt. 3 &mdash; A Living World

Previous: [Worldcrafting from Scratch Pt. 2 &mdash; New Beginnings](wb-02-new-beginnings.md)

### Table of Contents

- [Worldcrafting from Scratch Pt. 3 — A Living World](#worldcrafting-from-scratch-pt-3--a-living-world)
    - [Table of Contents](#table-of-contents)
  - [The passage of time](#the-passage-of-time)
    - [Time periods](#time-periods)

<br/>

As of [part 1](wb-02-new-beginnings.md), you have everything you need to make a responsive, engaging world. But there is still a problem of _stasis_. It's a thing that only exists insofar as players interact with it, but is nothing apart from that interaction.

To fix that, we need to make the world dynamic and changing. Things _happen_, and those happenings have nothing to do with the players. They're walking into a world already in motion; a world that is _alive_.

## The passage of time

The only indication, at the moment, that the MUD continues to run while a player sits on their prompt are the messages about the weather ("The rain stopped.") and the time of day ("The day has begun."). These events have very little to do with the game world, itself, except that night-time makes some sectors "dark".

Most "modern" MUDs (from, say, 1993 and after) have so-called "night descriptions". They are easy to implement, and can have a dramatic effect world _if_ your builders are willing the put in the work.

Mud98 takes a much more customizable approach by allowing builders to create ad hoc time periods by room; each of which can have its own bespoke description. 

### Time periods

Let's head back to the first room we made in Part 1:

```
redit 12000
```

Use the `PERIOD` command to create an ad hoc block of time. With no arguments, we get this:

```
Syntax:
  period list
  period add <name> [start end]
  period desc <name>
  period delete <name>
  period set <name> <start> <end>
  period rename <name> <new name>
  period format <name> [desc|enter|exit]
  period enter <name>
  period exit <name>
  period suppress <on|off>
```

There's a lot here. Let's try `PERIOD ADD`:

```
Specify the name of the time period to add to this room.
You may optionally specify start and end hours, or use one of the presets: day, dusk, night, dawn.
```

We can set periods to any hour we want; but for the sake of easy demonstration, let's start with:

```
period add night
```

The period name `night` is a preset that defines a period between 20:00 in the evening, and 04:00 the next morning. We also get dumped into a string editor. This is the room description for that time period. Let's add some ludicrously over-wrought prose:

```
  The night-time forest is obscured by gloomy darkness. Nevertheless, you can 
hear the sounds of life around you. The path through the forest continues east;
but impenetrable darkness hides the way.
@
```

To test these changes, simply reload the room:

```
reload room
```

...and then you wait. After you see the message "The night has begun", `LOOK`:

```
The Awakening [Room 12000]
  The night-time forest is obscured by gloomy darkness. Nevertheless, you can 
hear the sounds of life around you. The path through the forest continues east;
but impenetrable darkness hides the way.
```

This is a naive implementation establishing parity with the old "night desc" snippets. But we can do so much more:

```
period add dusk
  It's hard to see past the closest trees in the waning light. You can still
make out a path leading east.
@
```

Now we have a _third_ room description; one that can only be seen between 19:00 and 19:59 in the evening.