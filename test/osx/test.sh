#! /bin/bash

basepath="/Applications/ET"
path_to_game_exec="$basepath/ET.app/Contents/MacOS/ET"

mod_name="etrun"
pk3_name="etrun.pk3"

apimodule_name='api_mac'
qagame_name='qagame_mac'
cgame_name='cgame_mac'
ui_name='ui_mac'

path_to_api="$basepath/$mod_name/$apimodule_name"

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
mkdir "$basepath/$mod_name"
cp -f $pk3_name "$basepath/$mod_name"
rm -f $pk3_name
cp -rf _build/$qagame_name "$basepath/$mod_name"
cp -rf _build/$apimodule_name "$basepath/$mod_name"
read -p "Press ENTER to start game..."
"$path_to_game_exec" +set fs_game $mod_name +set dedicated 2 +map goldrush +set g_useAPI 0 +set g_APImodulePath "$path_to_api"
