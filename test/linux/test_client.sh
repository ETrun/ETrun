#! /bin/bash

basepath="/usr/local/games/enemy-territory"
path_to_game_exec="/usr/local/games/enemy-territory/et.x86"

homepath=/home/$USER/.etwolf

mod_name="etrun"
pk3_name="etrun.pk3"

apimodule_name='APImodule.so'
qagame_name='qagame.mp.i386.so'
cgame_name='cgame.mp.i386.so'
ui_name='ui.mp.i386.so'

path_to_api="$homepath/$mod_name/$apimodule_name"

cp -rf _build/$cgame_name $mod_name/
cp -rf _build/$ui_name $mod_name/

rm -f $pk3name
cd etrun
zip -r ../$pk3_name *
rm -rf $cgame_name
rm -rf $ui_name
rm -rf $qagame_name
rm -rf $apimodule_name
cd ..
mkdir "$homepath/$mod_name"
cp -f $pk3_name "$homepath/$mod_name"
rm -f $pk3_name
cp -rf _build/$qagame_name "$homepath/$mod_name"
cp -rf _build/$apimodule_name "$homepath/$mod_name"
read -p "Press ENTER to start game..."
"$path_to_game_exec" +set fs_basepath $basepath +set fs_game $mod_name +map goldrush +set g_useAPI 0 +set g_APImodulePath "$path_to_api" +set developer 1
