#! /bin/bash

BUILD_DIR=_build

# Clean
make distclean

# Compile for windows
./configure --build i686-pc-linux-gnu --host i586-mingw32msvc CFLAGS="-no-undefined -DOS_WINDOWS"
make

if [ $? != 0 ]; then
  exit
fi

strip -s src/cgame/.libs/libcgame.dll
strip -s src/game/.libs/libgame.dll
strip -s src/ui/.libs/libui.dll
strip -s src/APImodule/.libs/libapi.dll
mv src/cgame/.libs/libcgame.dll $BUILD_DIR/cgame_mp_x86.dll
mv src/game/.libs/libgame.dll $BUILD_DIR/qagame_mp_x86.dll
mv src/ui/.libs/libui.dll $BUILD_DIR/ui_mp_x86.dll
mv src/APImodule/.libs/libapi.dll $BUILD_DIR/APImodule.dll
