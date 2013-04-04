#! /bin/bash

#
# TODO
# -check script is started from etrun root dir
#

#
# Settings (do not edit)
#
USE_API=0
CLIENT_MODE=0
DEVELOPER=0
OSX_CONFIG_FILE='osx.config'
LINUX_CONFIG_FILE='linux.config'
OTHER_CONFIG_FILE='other.config'
USE_ETL=0
USE_VALGRIND=0
USE_DEBUGGER=0
INSTALL_FILES=''

#
# Parse options
#
function parse_options() {
	while getopts ":ahdm:cvlgi:" opt; do
	  	case $opt in
		  	h)
				show_usage
				exit 0
				;;
		    a)
				USE_API=1
				;;
			c)
				CLIENT_MODE=1
				;;
			d)
				DEVELOPER=1
				;;
			g)
				USE_DEBUGGER=1
				;;
			i)
				INSTALL_FILES=$OPTARG
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
	echo 'Usage: '`basename $0` ' [options...]'
	echo 'Options:'
	echo ' -a 		Use API'
	echo ' -c		Start game in client mode'
	echo ' -d 		Enable DEVELOPER mode'
	echo ' -g 		Start game with a debugger'
	echo ' -h		Show this help'
	echo ' -i PK3NAME	Install the pk3 given in argument, qagame and custommapscripts into homepath'
	echo ' -l		Use ET: Legacy'
	echo ' -m MAPNAME	Set map'
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
# Init some variables
#
function init() {
	# Set game binary
	if [ $USE_ETL -eq 1 ]; then
		HOMEPATH=$etl_home_path
		BASEPATH=$etl_base_path
		if [ $CLIENT_MODE -eq 0 ]; then
			GAME_PATH=$etl_dedicated_binary_path
		else
			GAME_PATH=$etl_binary_path
		fi
	else
		HOMEPATH=$et_home_path
		BASEPATH=$et_base_path
		if [ $CLIENT_MODE -eq 0 ]; then
			GAME_PATH=$et_dedicated_binary_path
		else
			GAME_PATH=$et_binary_path
		fi
	fi
}

#
# Install pk3, qagame and custommapscripts
#
function install() {
	# Check argument
	if [ ! -f build/$INSTALL_FILES ]; then
		echo '[ko]'
		echo "Error: cannot find build/$INSTALL_FILES"
		exit 1
	fi

	# Clean homepath and basepath
	if [ $USE_ETL -eq 1 ]; then
		rm -rf $etl_base_path/$mod_name
		rm -rf $etl_home_path/$mod_name
	else
		rm -rf $et_base_path/$mod_name
		rm -rf $et_home_path/$mod_name
	fi

	# Make etrun/ dir in homepath
	mkdir -p $HOMEPATH/$mod_name

	# Install pk3 into homepath
	cp -f build/$INSTALL_FILES $HOMEPATH/$mod_name
	if [ $? -ne 0 ]; then
		echo '[ko]'
		echo "Error: failed to install build/$INSTALL_FILES into $HOMEPATH/$mod_name"
		exit 1
	fi

	# Install qagame into homepath
	cp -f build/$mod_name/$qagame_name $HOMEPATH/$mod_name
	if [ $? -ne 0 ]; then
		echo '[ko]'
		echo "Error: failed to install build/$mod_name/$qagame_name into $HOMEPATH/$mod_name"
		exit 1
	fi

	# Install custom mapscripts into homepath
	mkdir -p $HOMEPATH/$mod_name/custommapscripts
	cp -f $mod_name/custommapscripts/* $HOMEPATH/$mod_name/custommapscripts

	# Install GeoIP
	cp libs/geoip/GeoIP.dat $HOMEPATH/$mod_name/
}

#
# Install API
#
function install_API() {
	cp -f build/$mod_name/$APImodule_name $HOMEPATH/$mod_name 2> /dev/null
	if [ $? -ne 0 ]; then
		echo '[ko]'
		echo "Error: failed to copy build/$mod_name/$APImodule_name to $HOMEPATH/$mod_name"
		exit 1
	fi
}

#
# Print summary of all options before starting game
#
function print_summary() {
	echo '###################################'
	echo '###      Current settings       ###'
	echo '###################################'

	echo "Base path: $BASEPATH"
	echo "Home path: $HOMEPATH"

	if [ $CLIENT_MODE -eq 1 ]; then
		echo 'Game mode: CLIENT'
	else
		echo 'Game mode: SERVER'
	fi

	echo "Map: $default_map"

	if [ $USE_API -eq 0 ]; then
		echo 'API: no'
	else
		echo 'API: YES'
	fi

	if [ $DEVELOPER -eq 0 ]; then
		echo 'Developper mode: disabled'
	else
		echo 'Developper mode: ENABLED'
	fi

	if [ $USE_VALGRIND -eq 0 ]; then
		echo 'Valgrind: disabled'
	else
		echo 'Valgrind: ENABLED'
	fi

	if [ $USE_DEBUGGER -eq 0 ]; then
		echo 'Debugger: disabled'
	else
		echo 'Debugger: ENABLED'
	fi
	echo '###################################'
}

#
# Function to start game
#
function start_game() {
	# Prepare game args for et/etded
	if [ $CLIENT_MODE -eq 0 ]; then
		GAME_ARGS="+set fs_game $mod_name +set fs_basePath $BASEPATH +set fs_homePath $HOMEPATH +set g_useAPI $USE_API +set g_APImodulePath $HOMEPATH/$mod_name/$APImodule_name +set developer $DEVELOPER +map $default_map"
	else
		GAME_ARGS="+set fs_game $mod_name +set fs_basePath $BASEPATH +set fs_homePath $HOMEPATH +set developer $DEVELOPER +set com_hunkMegs 128"
	fi

	# Workaround for OSX where there isn't ETDED binary
	if [[ $OS == "Darwin" && $CLIENT_MODE -eq 0 ]]; then
		GAME_ARGS="$GAME_ARGS +set dedicated 1"
	fi

	if [ $USE_VALGRIND -eq 1 ]; then
		$valgrind_command_line $GAME_PATH $GAME_ARGS
	elif [ $USE_DEBUGGER -eq 1 ]; then
		$debugger_command_line $GAME_PATH $GAME_ARGS
	else
		echo $GAME_PATH $GAME_ARGS
		read -p 'Press ENTER to start game...'
		eval "$GAME_PATH $GAME_ARGS"
	fi
}

#
# Main
#
read_config

parse_options "$@"

init

if [ ! -z $INSTALL_FILES ]; then
	echo -n ' Installing files...'
	install
	echo '[ok]'
fi

if [ $USE_API -eq 1 ]; then
	echo -n ' Installing API...'
	install_API
	echo '[ok]'
fi

print_summary

start_game
