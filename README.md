# CMDragons autoref for RoboCup SSL

This is the RoboCup SSL autoref by [CMDragons](https://www.cs.cmu.edu/~robosoccer/small/).

## Running

The autoref expects to be run with the refbox and (optionally) consensus
algorithm running at the same time; the autoref does not send raw referee
command packets, only refbox remote control requests.

- to compile: `mkdir build; cd build; cmake ..; make -j$(nproc)`
- to run: `bin/autoref`

The autoref executable also takes the following arguments:

- `-p, --passive`: don't try sending remote control commands at all
- `-b, --divb`: run for division B instead of division A
- `-n, --nocon`: send directly to the refbox (port 10007) instead of the consensus program (10008)

## Handled rules
- awarding indirect free kicks after the ball exits, is shot too fast, or is dribbled too far
- awarding goals and setting up kickoffs
- halting for halftime and the end of the game
- force-starting when the ball gets stuck or a team fails to take a kick
- multiple defenders

## Missing rules
- detecting more infractions
- running extra time and penalty shootouts

## Other issues/missing features
- unpolished tracking and ball touching detection
- no ability to be guided or overridden by a human
- no ability to handle timeouts
- extremely minimal user interface

## Network configuration
- To shift all the ports used by an additive offset, put the offset into the
  file `shared/PORT_OFFSET` and recompile. If the ports are not all offset by
  the same amount, edit the `*Port` variables in `shared/udp.h` and recompile.
