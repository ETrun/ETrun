#! /bin/bash

#
# Settings
#
basepath='/Applications/ET'
path_to_game_exec="$basepath/ET.app/Contents/MacOS/ET"
mod_name='etrun'
pk3_name='etrun.pk3'
apimodule_name='api_mac'
qagame_name='qagame_mac'
cgame_name='cgame_mac'
ui_name='ui_mac'
path_to_api="$basepath/$mod_name/$apimodule_name"
default_map='goldrush'
use_api=0
dedicated=1
developer=0

#
# Parse options
#
function parse_options() {
	while getopts ":ahdm:c" opt; do
	  	case $opt in
		  	h)
				show_usage
				exit 0
				;;
		    a)
				use_api=1
				;;
			c)
				dedicated=0
				;;
			d)
				developer=1
				;;
			m)
				default_map=$OPTARG
				;;
		    \?)
				echo "Invalid option: -$OPTARG"
				;;
	  	esac
	done
}

#
# Show usage
#
function show_usage() {
	echo 'Usage: '`basename $0` ' [options...]'
	echo 'Options:'
	echo ' -a 		Use API'
	echo ' -c		Start game in client mode'
	echo ' -d 		Enable developer mode'
	echo ' -h		Show this help'
	echo ' -m NAPNAME	Set map'
}

#
# Function used to clean game
#
function clean_game() {
	rm -rf $basepath/$mod_name
}

#
# Make pk3 function
#
function make_pk3() {
	cp -rf build/$mod_name/$cgame_name $mod_name
	cp -rf build/$mod_name/$ui_name $mod_name
	rm -f $pk3name
	cd $mod_name
	zip -qr ../$pk3_name *
	rm -rf $cgame_name
	rm -rf $ui_name
	rm -rf $qagame_name
	rm -rf $apimodule_name
	cd ..
}

#
# Install mod function
#
function install() {
	# Make etrun/ dir
	mkdir -p $basepath/$mod_name
	# Install client files
	cp -f $pk3_name $basepath/$mod_name
	# Install server files
	cp -rf build/$mod_name/$qagame_name $basepath/$mod_name
	if [ $use_api -eq 1 ]; then
		cp -rf build/$mod_name/$apimodule_name $basepath/$mod_name 2> /dev/null
		if [ $? -ne 0 ]; then
			echo '[ko]'
			echo "Error: failed to copy build/$mod_name/$apimodule_name to $basepath/$mod_name"
			exit 1
		fi
	fi
	# Clean
	rm -f $pk3_name
	# Install custom mapscripts
	mkdir -p $basepath/$mod_name/custommapscripts
	cp -rf etrun/custommapscripts/* $basepath/$mod_name/custommapscripts
}

#
#
#
function print_summary() {
	echo '###################################'
	echo '###      Current settings       ###'
	echo '###################################'
	if [ $dedicated -eq 0 ]; then
		echo 'Game will be started in CLIENT mode'
	else
		echo 'Game will be started in SERVER mode'
	fi

	echo "Map: $default_map"
	if [ $use_api -eq 1 ]; then
		echo 'API will be used'
	fi

	if [ $developer -eq 0 ]; then
		echo 'Developer mode will be DISABLED'
	else
		echo 'Developer mode will be ENABLED'
	fi
	echo '###################################'
}

#
# Main
#
parse_options "$@"

echo -n ' Cleaning game...'
clean_game
echo '[ok]'

echo -n ' Building pk3...'
make_pk3
echo '[ok]'

echo -n ' Installing files...'
install
echo '[ok]'

print_summary

read -p 'Press ENTER to start game...'
$path_to_game_exec +set fs_game $mod_name +set dedicated $dedicated +map $default_map +set g_useAPI $use_api +set g_APImodulePath $path_to_api +set developer $developer
