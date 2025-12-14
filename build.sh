#!/bin/bash

export BOOST_ROOT=$HOME/boost/boost_1_84_0/dist
export BOOST_INCLUDEDIR=$BOOST_ROOT/include
export BOOST_LIBRARYDIR=$BOOST_ROOT/lib

export CPPFLAGS="-I$BOOST_INCLUDEDIR -I/opt/homebrew/include"
export LDFLAGS="-L$BOOST_LIBRARYDIR -L/opt/homebrew/lib"

# 1️⃣ Full clean of autotools outputs
make distclean || true

# 2️⃣ Remove cached autotools state (important)
rm -rf autom4te.cache

rm -rf src/.libs

./autogen.sh
./configure \
  --with-boost=$BOOST_ROOT \
  --with-boost-libdir=$BOOST_LIBRARYDIR \
  --without-miniupnpc \
  --without-gui \
  --disable-gui-tests \
  --enable-shared

make -j$(sysctl -n hw.ncpu)
#make

### for tarcin poblems

###CXXFLAGS="-fsanitize=address -g" CFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address" ./configure ...
###make -j$(nproc)

###
