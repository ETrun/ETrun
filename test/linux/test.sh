#! /bin/bash

path_to_mod='/home/me/.etwolf/etrun/'

cp -f _build/cgame_mp_x86.dll etrun/
cp -f _build/ui_mp_x86.dll etrun/

cp -f _build/cgame.mp.i386.so etrun/
cp -f _build/ui.mp.i386.so etrun/

rm -f etrun.pk3
cd etrun
zip -r ../etrun.pk3 *
rm -f *.dll
rm -f *.so
cd ..
cp -f etrun.pk3 $path_to_mod
rm -f etrun.pk3
cp -f _build/qagame.mp.i386.so $path_to_mod
read -p "Press ENTER to start game..."
/usr/local/games/enemy-territory/etded.x86 +set fs_basepath "/usr/local/games/enemy-territory" +set fs_game etrun +devmap goldrush
