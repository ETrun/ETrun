#! /bin/bash

BUILD_DIR=_build
QAGAME_LIB=qagame_mac
API_LIB=api_mac
CGAME_LIB=cgame_mac
UI_LIB=ui_mac

# Clean
make distclean

rm -f $BUILD_DIR/*_mac
rm -rf $BUILD_DIR/*_mac.bundle

# Compile for linux
./configure CFLAGS="-m32"
make

if [ $? != 0 ]; then
  exit
fi

strip -x src/cgame/.libs/libcgame.dylib
strip -x src/game/.libs/libgame.dylib
strip -x src/ui/.libs/libui.dylib
strip -x src/APImodule/.libs/libapi.dylib
mv src/cgame/.libs/libcgame.dylib $BUILD_DIR/$CGAME_LIB
mv src/game/.libs/libgame.dylib $BUILD_DIR/$QAGAME_LIB
mv src/ui/.libs/libui.dylib $BUILD_DIR/$UI_LIB
mv src/APImodule/.libs/libapi.dylib $BUILD_DIR/$API_LIB

cd $BUILD_DIR
../makebundle.sh qagame
../makebundle.sh cgame
../makebundle.sh ui
cd ..
