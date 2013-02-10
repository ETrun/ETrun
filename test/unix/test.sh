#! /bin/bash

#
# Settings (do not edit)
#
USE_API=0
DEDICATED=1
DEVELOPER=0
OSX_CONFIG_FILE='osx.config'
LINUX_CONFIG_FILE='linux.config'
OTHER_CONFIG_FILE='other.config'
USE_ETL=0
USE_VALGRIND=0

#
# Parse options
#
function parse_options() {
	while getopts ":ahdm:cvl" opt; do
	  	case $opt in
		  	h)
				show_usage
				exit 0
				;;
		    a)
				USE_API=1
				;;
			c)
				DEDICATED=0
				;;
			d)
				DEVELOPER=1
				;;
			l)
				USE_ETL=1
				;;
			m)
				default_map=$OPTARG
				;;
			v)
				USE_VALGRIND=1
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
	echo ' -d 		Enable DEVELOPER mode'
	echo ' -h		Show this help'
	echo ' -l		Use ET: Legacy'
	echo ' -m NAPNAME	Set map'
	echo ' -v 		Use Valgrind to check memory leaks'
}

#
# Function used to read and load config
#
function read_config() {
	OS=$(uname)
	WD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

	if [ $OS == "Darwin" ]; then
		echo -n " OSX detected, loading $OSX_CONFIG_FILE..."
		CONFIG_FILE=$OSX_CONFIG_FILE
	elif [ $OS == "Linux" ]; then
		echo -n " Linux detected, loading $LINUX_CONFIG_FILE..."
		CONFIG_FILE=$LINUX_CONFIG_FILE
	else
		echo -n " Unkown OS, loading $OTHER_CONFIG_FILE..."
		CONFIG_FILE=$OTHER_CONFIG_FILE
	fi

	if [ ! -f $WD/$CONFIG_FILE ]; then
		echo "[ko] Make sure $PWD/$CONFIG_FILE exists"
		exit 1
	fi
	echo '[ok]'

	# Load file
	source $WD/$CONFIG_FILE
}

#
# Function used to clean game
#
function clean_game() {
	if [ $USE_ETL -eq 1 ]; then
		rm -rf $etl_home_path/$mod_name
	else
		rm -rf $et_home_path/$mod_name
	fi
}

#
# Make pk3 function
#
function make_pk3() {
	cp -f build/$mod_name/$cgame_name $mod_name
	cp -f build/$mod_name/$ui_name $mod_name
	rm -f $pk3_name
	cd $mod_name
	zip -qr ../$pk3_name *
	rm -f $cgame_name
	rm -f $ui_name
	rm -f $qagame_name
	rm -f $APImodule_name
	cd ..
}

#
# Install mod function
#
function install() {
	# Set game binary
	if [ $USE_ETL -eq 1 ]; then
		HOMEPATH=$etl_home_path
		BASEPATH=$etl_base_path
		if [ $DEDICATED -eq 1 ]; then
			GAME_PATH=$etl_dedicated_binary_path
		else
			GAME_PATH=$etl_binary_path
		fi
	else
		HOMEPATH=$et_home_path
		BASEPATH=$et_base_path
		if [ $DEDICATED -eq 1 ]; then
			GAME_PATH=$et_dedicated_binary_path
		else
			GAME_PATH=$et_binary_path
		fi
	fi

	# Make etrun/ dir
	mkdir -p $HOMEPATH/$mod_name

	# Install client files
	cp -f $pk3_name $HOMEPATH/$mod_name

	# Install server files
	cp -f build/$mod_name/$qagame_name $HOMEPATH/$mod_name

	if [ $USE_API -eq 1 ]; then
		cp -f build/$mod_name/$APImodule_name $HOMEPATH/$mod_name 2> /dev/null
		if [ $? -ne 0 ]; then
			echo '[ko]'
			echo "Error: failed to copy build/$mod_name/$APImodule_name to $HOMEPATH/$mod_name"
			exit 1
		fi
	fi

	# Clean
	rm -f $pk3_name

	# Install custom mapscripts
	mkdir -p $HOMEPATH/$mod_name/custommapscripts
	cp -f $mod_name/custommapscripts/* $HOMEPATH/$mod_name/custommapscripts
}

#
#
#
function print_summary() {
	echo '###################################'
	echo '###      Current settings       ###'
	echo '###################################'

	echo "Base path: $BASEPATH"
	echo "Home path: $HOMEPATH"

	if [ $DEDICATED -eq 0 ]; then
		echo 'Game mode: CLIENT'
	else
		echo 'Game mode: SERVER'
	fi

	echo "Map: $default_map"

	if [ $USE_API -eq 0 ]; then
		echo 'API: no'
	else
		echo 'API: yes'
	fi

	if [ $DEVELOPER -eq 0 ]; then
		echo 'Developper mode: DISABLED'
	else
		echo 'Developper mode: ENABLED'
	fi

	if [ $USE_VALGRIND -eq 0 ]; then
		echo 'Valgrind: DISABLED'
	else
		echo 'Valgrind: ENABLED'
	fi
	echo '###################################'
}

function start_game() {
	if [ $USE_VALGRIND -eq 1 ]; then
		$valgrind_command_line $GAME_PATH \
		+set fs_game $mod_name \
		+set fs_basePath $BASEPATH \
		+set fs_homePath $HOMEPATH \
		+set dedicated $DEDICATED \
		+set g_useAPI $USE_API \
		+set g_APImodulePath $HOMEPATH/$mod_name/$APImodule_name \
		+set developer $DEVELOPER \
		+map $default_map
	else
		$GAME_PATH \
		+set fs_game $mod_name \
		+set fs_basePath $BASEPATH \
		+set fs_homePath $HOMEPATH \
		+set dedicated $DEDICATED \
		+set g_useAPI $USE_API \
		+set g_APImodulePath $HOMEPATH/$mod_name/$APImodule_name \
		+set developer $DEVELOPER \
		+map $default_map
	fi
}

#
# Main
#
read_config

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
start_game
