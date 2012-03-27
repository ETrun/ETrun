#! /bin/bash

BUILD_DIR=_build

# Clean
make distclean

# Compile for linux
./configure CFLAGS="-m32"
make

if [ $? != 0 ]; then
  exit
fi

strip -x src/cgame/.libs/libcgame.dylib
strip -x src/game/.libs/libgame.dylib
strip -x src/ui/.libs/libui.dylib
mv src/cgame/.libs/libcgame.dylib $BUILD_DIR/cgame_mac
mv src/game/.libs/libgame.dylib $BUILD_DIR/qagame_mac
mv src/ui/.libs/libui.dylib $BUILD_DIR/ui_mac
