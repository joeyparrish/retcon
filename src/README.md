# Requirements

 * g++
 * pigpio

# Compiling on raspbian

```sh
# Install pigpio
wget abyz.co.uk/rpi/pigpio/pigpio.tar
tar xf pigpio.tar
cd PIGPIO
make
sudo make install

# Build controller app
cd /path/to/retcon/src
make
```

# Cross-compiling from Ubuntu

```sh
sudo apt-get install g++-arm-linux-gnueabi
CROSS_PREFIX=arm-linux-gnueabi

# Install pigpio
wget abyz.co.uk/rpi/pigpio/pigpio.tar
tar xf pigpio.tar
cd PIGPIO
make \
  CC=$CROSS_PREFIX-gcc \
  SHLIB="$CROSS_PREFIX-gcc -shared" \
  AR=$CROSS_PREFIX-ar \
  RANLIB=$CROSS_PREFIX-ranlib \
  SIZE=$CROSS_PREFIX-size \
  STRIPLIB=$CROSS_PREFIX-strip \
sudo make prefix=/usr/$CROSS_PREFIX install

# Build controller app
cd /path/to/retcon/src
make CXX=$CROSS_PREFIX-g++
```

