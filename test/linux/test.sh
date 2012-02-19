#! /bin/bash

path_to_mod='/home/me/.etwolf/etrun/'

cp -f _build/qagame.mp.i386.so $path_to_mod
read -p "Press ENTER to start game..."
/usr/local/games/enemy-territory/etded.x86 +set fs_basepath "/usr/local/games/enemy-territory" +set fs_game etrun +devmap goldrush
