# CMDragons autoref for RoboCup SSL

This is the RoboCup SSL autoref competition entry from
[CMDragons](https://www.cs.cmu.edu/~robosoccer/small/).

## Description

The autoref is intended to be able to run whole games autonomously, except for
the placement of the ball for kicks. It can also run in evaluation mode for the
[technical challenge](http://wiki.robocup.org/wiki/Small_Size_League/RoboCup_2016/Technical_Challenges/Autoref_Challenge).
In that mode, it listens to commands from an SSL refbox and changes its own
state accordingly, and does not signal for game restarts or the ends of game
periods.

When started as a fully autonomous referee, the autoref will transmit the `STOP`
command and wait for robots of both teams to be present and moving, then
transmit the command for kickoff and begin the first half. It will transmit the
`HALT` command for halftime, then begin and end the second half similarly. When
the game needs to be stopped, the autoref will wait until the ball is stationary
near the desired reset location and the robots have stabilized (with a timeout),
and then transmit the commands to restart the game.

The autoref communicates according to the published SSL protocols, reading the
SSL-Vision double-size field protocol (the old geometry format, on port 10005)
and publishing the new protobuf-based referee protocol. It prints information
about what is happening to the terminal. When the ball needs to be moved to a
particular location for a kick, the coordinates will be printed and highlighted
in the terminal.

## Compiling and running
Tested with

- Linux 3.13 (Ubuntu 14.04)
- CMake 2.8
- libprotobuf-dev and protobuf-compiler 2.5.0
- g++ 4.8

Commands:

- to compile: `mkdir build; cd build; cmake ..; make`
- to run as a full autoref: `bin/autoref`
- to run in evaluation mode: `bin/autoref --eval`

## Handled rules
- awarding indirect free kicks after the ball exits, is shot too fast, or is dribbled too far
- awarding goals and setting up kickoffs
- halting for halftime and the end of the game
- force-starting when the ball gets stuck or a team fails to take a kick

## Missing rules
- detecting more infractions (pushing, multiple defenders, etc.)
- running extra time and penalty shootouts

## Other issues/missing features
- unpolished tracking and ball touching detection
- no ability to be guided or overridden by a human
- no ability to handle timeouts
- extremely minimal user interface
