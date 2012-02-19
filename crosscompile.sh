#! /bin/bash

BUILD_DIR=_build

# Clean
make distclean

# Compile for unix
./configure CFLAGS="-Wno-enum-compare -Wno-format-security -Wno-switch -Wno-array-bounds"
make
strip -s src/cgame/.libs/libcgame.so
strip -s src/game/.libs/libgame.so
strip -s src/ui/.libs/libui.so
mv src/cgame/.libs/libcgame.so $BUILD_DIR/cgame.mp.i386.so
mv src/game/.libs/libgame.so $BUILD_DIR/qagame.mp.i386.so
mv src/ui/.libs/libui.so $BUILD_DIR/ui.mp.i386.so

# Compile for windows
make distclean
./configure --build i686-pc-linux-gnu --host i586-mingw32msvc CFLAGS="-no-undefined"
make
strip -s src/cgame/.libs/libcgame.dll
strip -s src/game/.libs/libgame.dll
strip -s src/ui/.libs/libui.dll
mv src/cgame/.libs/libcgame.dll $BUILD_DIR/cgame_mp_x86.dll
mv src/game/.libs/libgame.dll $BUILD_DIR/qagame_mp_x86.dll
mv src/ui/.libs/libui.dll $BUILD_DIR/ui_mp_x86.dll
