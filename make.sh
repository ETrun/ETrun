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
CROSS_COMPILE32=0

#
# Parse options
#
function parse_options() {
  while getopts ":hdcv" opt; do
    case $opt in
      h)
        show_usage
        exit 0
        ;;
      d)
        DEBUG=1
        ;;
      c)
        CROSS_COMPILE32=1
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
  echo '  -c               Cross compile for x86'
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
  echo "Building in $([ $DEBUG -eq 1 ] && echo 'Debug' || echo 'Release') mode"

  # Build mod
  mkdir -p $BUILD_DIR
  cd $BUILD_DIR

  # Prepare CMake params
  if [ $DEBUG -eq 1 ]; then
    CMAKE_PARAMS='-DCMAKE_BUILD_TYPE=Debug'
  else
    CMAKE_PARAMS='-DCMAKE_BUILD_TYPE=Release'
  fi
  if [ $VERBOSE -eq 1 ]; then
    CMAKE_PARAMS="$CMAKE_PARAMS -DCMAKE_VERBOSE_MAKEFILE=TRUE"
  fi

  if [ $CROSS_COMPILE32 -eq 1 ]; then
    CMAKE_PARAMS="$CMAKE_PARAMS -DCROSS_COMPILE32=ON"
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
    echo 'An error occurred while running CMake'
    exit 1
  fi

  # Run make except on Windows
  if [[ ! $OS == MINGW* ]]; then
    make
    if [ $? -ne 0 ]; then
      echo 'An error occurred while running make'
      exit 1
    fi
  fi
}

function check_dependency() {
  if ! command -v "$1" &> /dev/null; then
    echo "Warning: dependency unmet '$1'"
  fi
}

#
# Main
#
detect_os
check_dependency $CMAKE
check_dependency "zip"
parse_options "$@"
clean
build
