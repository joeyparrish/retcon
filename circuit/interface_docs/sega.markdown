# Sega Genesis and Master System Controller Interface

## Connector

The connector is a slight variation on a normal DB9 connector.  The socket on
the game console is male, and the plug on the end of the controller is female.

A normal DB9 female plug won't fit into the Sega controller port.  The socket
on the game console is a little too deep, and the metal shielding on the female
plug makes the whole thing too wide.

It seems that the best way to connect our circuit to the Sega is via an
extension cable that is meant for Sega Genesis.

![female DB9 plug](images/female_db9.jpg)
![Sega Genesis controller socket](images/genesis_socket.jpg)
![Sega Genesis extension cable](images/genesis_extension.jpg)


## Sega Master System Controller

The Master System controller has a D-pad and two buttons.  The controller
interface is a 5V parallel interface with active-low signals.  Pins are
pulled high in the controller and pushing a button connects the pin to ground.

![Sega Master System Controller](images/master_system_controller.jpg)
![DB9 connector pins](images/db9_pins.jpg)

|pin|signal               |
|---|---------------------|
|1  |Up                   |
|2  |Down                 |
|3  |Left                 |
|4  |Right                |
|5  |(not used)           |
|6  |Button 1             |
|7  |+5V (from console)   |
|8  |Ground (from console)|
|9  |Button 2             |


## Sega Genesis Controller

The Genesis controller has a D-pad, a start button, and buttons A, B, and C.
The controller interface is backward-compatible with the Sega Master System,
so a Genesis controller should work on a Master System.

To add more buttons while maintaining compatibility, the Sega Genesis adds a
select signal to the interface.  Based on the state of the select pin, the
controller multiplexes additional button states onto existing pins.  The select
pin is pulled high in the controller so that it is not left floating when the
controller is connected to a Master System.

![Sega Genesis Controller](images/genesis_controller.jpg)
![DB9 connector pins](images/db9_pins.jpg)

|pin|signal (select high)      |signal (select low)          |
|---|--------------------------|-----------------------------|
|1  |Up                        |Up                           |
|2  |Down                      |Down                         |
|3  |Left                      |Ground                       |
|4  |Right                     |Ground                       |
|5  |Select (from console, +5V)|Select (from console, ground)|
|6  |B Button                  |A Button                     |
|7  |+5V (from console)        |+5V (from console)           |
|8  |Ground (from console)     |Ground (from console)        |
|9  |C Button                  |Start Button                 |


### External References

 - http://gamesx.com/wiki/doku.php?id=controls:megadrive_genesis_controller_3
 - http://www.haku.co.uk/pics/SegaJoypadPinout.jpg
 - https://www.cs.cmu.edu/~chuck/infopg/segasix.txt
