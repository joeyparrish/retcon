![RetCon](logo.png)

# Retro Console Controller

Use comfortable wireless PS3 controllers on your old Sega or Nintendo system.

The PS3 controllers pair with a Raspberry Pi, which translates the controller
inputs to the game console through an external circuit.

[The repository][] hosts the schematics, board layouts, and source code.

For a list of known issues, see our [issue tracker][].

[The repository]: https://github.com/cashpipeplusplus/retcon
[issue tracker]: https://github.com/cashpipeplusplus/retcon/issues


## Software

RetCon consists of three pieces of software that run on a Raspberry Pi:

 * retcon - reads events from the gamepad and toggle GPIO pins
 * trusting-steed - waits for gamepads and marks them as "trusted"
 * sixpair - waits for PS3 controllers over USB and pairs them for bluetooth

These run on the Raspberry Pi, and I do not recommend cross-compiling them.
To build them, check out the source on Raspbian, install the deps, then build
the debian package:

```sh
git clone https://github.com/cashpipeplusplus/retcon
sudo apt-get install libusb-dev pigpio python-gobject python-dbus
cd retcon/src/
fakeroot ./debian/rules binary
```

To install on Raspbian:

```sh
sudo dpkg -i ../retcon_0.0.1-1_armhf.deb
```


## Raspberry Pi SD card image

The SD card image is generated using a fork of a tool called pi-gen, which is
included as a submodule.

From retcon, you can check out the pi-gen code like this:

```sh
git submodule update --init --recursive
```

To build the SD image, first you must build the retcon debian package from
Raspbian.  Then, copy the deb file to your Linux machine and build the SD image
there.  Assuming the deb file is in the retcon folder, run this:

```sh
cd pi-gen
sudo ./build.sh ../retcon_0.0.1-1_armhf.deb
```

The output will appear in `deploy/`.


## Pre-built SD card image

Pre-built SD card images are available from the [releases][] page.

[releases]: https://github.com/cashpipeplusplus/retcon/releases


## Hardware

The Raspberry Pi communicates with the game console through an external
circuit.  Schematics and board layouts for this extra hardware can be found
in the `circuit/` folder.  The board file can be uploaded directly to
[DirtyPCBs][] if you want the boards printed for you.  More details can be
found in `circuit/README.md`.

[DirtyPCBs]: http://dirtypcbs.com/
