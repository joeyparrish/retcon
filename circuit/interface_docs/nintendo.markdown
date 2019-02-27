# Nintendo Entertainment System and Super Nintendo Controller Interface

## Connectors

Sega made it so easy with DB9.  Nintendo's connectors are not only non-standard,
they changed the connector from the NES to the SNES.  The socket on the game
console is male, and the plug on the end of the controllers is female.

![female NES plug](images/nes_connector.jpg)
![female SNES plug](images/snes_connector.jpg)


## Controller interfaces

The NES controller has a D-pad and buttons A, B, start, and select.
The SNES controller adds X, Y, and L & R shoulder buttons to the NES layout.
The controller interface is a 5V serial interface with active-low signals.
Pins are pulled high in the controller and pushing a button connects the pin
to ground.

![NES controller](images/nes_controller.jpg)
![NES connector pins](images/nes_pins.png)
![SNES controller](images/snes_controller.jpg)
![SNES connector pins](images/snes_pins.png)

The serial interface works by latching all of the button states into a shift
register, then clocking out each individual button state.  The button states
form an 8-bit (NES) or 12-bit (SNES) word.

|pin|signal               |NES color¹|SNES color¹|
|---|---------------------|----------|-----------|
|1  |Ground (from console)|white     |black      |
|2  |(not used)           |          |           |
|3  |(not used)           |          |           |
|4  |Serial output        |black     |red        |
|5  |Latch (from console) |green     |yellow     |
|6  |Clock (from console) |yellow    |blue       |
|7  |+5V (from console)   |red       |green      |

¹: In the extension cords I bought... YMMV

|bit|NES button|SNES button|
|---|----------|-----------|
|0  |A         |B          |
|1  |B         |Y          |
|2  |Select    |Select     |
|3  |Start     |Start      |
|4  |Up        |Up         |
|5  |Down      |Down       |
|6  |Left      |Left       |
|7  |Right     |Right      |
|8  |(not used)|A          |
|9  |(not used)|X          |
|10 |(not used)|L          |
|11 |(not used)|R          |

Any extra bits in the shift register's parallel input must be pulled high.


### External References

 - http://gamesx.com/wiki/doku.php?id=controls:nes_snes_controller
 - http://pinoutguide.com/Game/snescontroller_pinout.shtml
