#! /bin/bash

BUILD_DIR=_build

# Clean
make distclean

# Compile for linux
./configure CFLAGS="-Wno-enum-compare -Wno-format-security -Wno-switch -Wno-array-bounds"
make

if [ $? != 0 ]; then
  exit
fi

strip -s src/cgame/.libs/libcgame.so
strip -s src/game/.libs/libgame.so
strip -s src/ui/.libs/libui.so
strip -s src/APImodule/.libs/libapi.so
mv src/cgame/.libs/libcgame.so $BUILD_DIR/cgame.mp.i386.so
mv src/game/.libs/libgame.so $BUILD_DIR/qagame.mp.i386.so
mv src/ui/.libs/libui.so $BUILD_DIR/ui.mp.i386.so
mv src/APImodule/.libs/libapi.so $BUILD_DIR/APImodule.so
