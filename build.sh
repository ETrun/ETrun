#! /bin/bash

#
# ETrun build script
#
# TODO: provide a debug option
#
#

#
# Settings
#
DEBUG=0
BUILD_DIR=build
CMAKE=cmake
BUILD_API=0

#
# Parse options
#
function parse_options() {
	while getopts ":had" opt; do
	  	case $opt in
		  	h)
				show_usage
				exit 0
				;;
			a)
				BUILD_API=1
				;;
			d)
				DEBUG=1
				;;
		    \?)
				echo "Invalid option: -$OPTARG"
				;;
	  	esac
	done
}

#
# Clean function
#
function clean() {
	rm -rf $BUILD_DIR
}

#
# Show usage
#
function show_usage() {
	echo 'Usage: '`basename $0` ' [options...]'
	echo 'Options:'
	echo ' -a		Build API'
	echo ' -d		Turn ON debug mode'
	echo ' -h		Show this help'
}

#
# Detect OS and set some vars
#
function detect_os() {
	OS=$(uname)

	if [ $OS == "Darwin" ]; then
		echo "OSX detected"
	else
		echo "Unkown OS"
	fi
}

#
# Build
#
function build() {
	# Build mod
	mkdir -p $BUILD_DIR
	cd $BUILD_DIR
	$CMAKE .. && make

	# Build API if asked
	if [ $BUILD_API -eq 1 ]; then
		cd ../libs/APImodule
		./build.sh
		cd ../..
	fi
}

#
# Main
#
parse_options "$@"
clean
detect_os
build