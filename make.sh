#! /bin/bash

#
# ETrun build script
#

#
# Settings
#
DEBUG=0
BUILD_DIR=build
MOD_NAME='etrun'
CMAKE=cmake
VERBOSE=0
TARGET_ARCHITECTURE=''

#
# Parse options
#
function parse_options() {
  while getopts ":hdvt:p" opt; do
      case $opt in
        h)
        show_usage
        exit 0
        ;;
      d)
        DEBUG=1
        ;;
      t)
        TARGET_ARCHITECTURE=$OPTARG
        ;;
      v)
        VERBOSE=1
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
  echo 'Usage: '`basename $0` ' [options...]'
  echo 'Options:'
  echo '  -d               Turn ON debug mode'
  echo '  -h               Show this help'
  echo '  -t ARCHITECTURE  Specify target architecture to build for (x86, x86_64)'
  echo '  -v               Enable verbose build'
}

#
# Detect OS and set some vars
#
function detect_os() {
  OS=$(uname)
}

#
# Build
#
function build() {
  # Build mod
  mkdir -p $BUILD_DIR
  cd $BUILD_DIR

  # Prepare CMake params
  if [ $DEBUG -eq 1 ]; then
    CMAKE_PARAMS='-D CMAKE_BUILD_TYPE=Debug'
  else
    CMAKE_PARAMS='-D CMAKE_BUILD_TYPE=Release'
  fi
  if [ $VERBOSE -eq 1 ]; then
    CMAKE_PARAMS="$CMAKE_PARAMS -D CMAKE_VERBOSE_MAKEFILE=TRUE"
  fi

  # Target architecture
  if [ ! $TARGET_ARCHITECTURE == '' ]; then
    # This option should be ignored on OSX because we build universal binaries
    if [ $OS == 'Darwin' ]; then
      echo "-t $TARGET_ARCHITECTURE ignored on OSX because universal binaries are being built"
    else
      CMAKE_PARAMS="$CMAKE_PARAMS -D TARGET_ARCHITECTURE=$TARGET_ARCHITECTURE"
    fi
  fi

  # For Windows, specify Visual studio version to use
  if [[ $OS == MINGW* ]]; then
    # Run CMake
    $CMAKE $CMAKE_PARAMS -G "Visual Studio 10" ..
  else
    # Run CMake
    $CMAKE $CMAKE_PARAMS ..
  fi

  if [ $? -ne 0 ]; then
    echo 'An error occured while running CMake'
    exit 1
  fi

  # Run make except on Windows
  if [[ ! $OS == MINGW* ]]; then
    make
    if [ $? -ne 0 ]; then
      echo 'An error occured while running make'
      exit 1
    fi
  fi
}

#
# Function used to strip modules
#
function strip_modules() {
  # Check debug mode is OFF
  if [ $DEBUG -eq 1 ]; then
    return
  fi

  echo -n 'Stripping modules...'
  if [ $OS == "Darwin" ]; then
    strip -x -S $MOD_NAME/cgame_mac
    strip -x -S $MOD_NAME/qagame_mac
    strip -x -S $MOD_NAME/ui_mac
  else
    strip -s $MOD_NAME/cgame*
    strip -s $MOD_NAME/qagame*
    strip -s $MOD_NAME/ui*
  fi
  echo '[done]'
}

#
# Main
#
parse_options "$@"
clean
detect_os
build
# Useless to strip after pk3 creation, should be done before
#strip_modules
