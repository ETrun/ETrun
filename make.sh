#! /bin/bash

#
# ETrun build script
#
#

#
# Settings
#
DEBUG=0
BUILD_DIR=build
MOD_NAME='etrun'
CMAKE=cmake
BUILD_API=0
MAKE_PK3=0
PK3_NAME="$MOD_NAME.pk3"
VERBOSE=0
TARGET_ARCHITECTURE=''

#
# Parse options
#
function parse_options() {
  while getopts ":hadvt:p" opt; do
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
      p)
        MAKE_PK3=1
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
  echo '  -a               Build API'
  echo '  -d               Turn ON debug mode'
  echo '  -h               Show this help'
  echo '  -p               Make a pk3 archive'
  echo '  -t ARCHITECTURE  Specify architecture to build for (x86, x86_64)'
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
    CMAKE_PARAMS="$CMAKE_PARAMS -D TARGET_ARCHITECTURE=$TARGET_ARCHITECTURE"
  fi

  # Run CMake
  $CMAKE $CMAKE_PARAMS ..
  if [ $? -ne 0 ]; then
    echo 'An error occured while running CMake'
    exit 1
  fi

  # Run make
  make
  if [ $? -ne 0 ]; then
    echo 'An error occured while running make'
    exit 1
  fi

  # Build API if asked
  if [ $BUILD_API -eq 1 ]; then
    cd ../libs/APImodule

    if [ $DEBUG -eq 1 ]; then
      APIMODULE_BUILD_PARAMS='-d'
    fi
    if [ $VERBOSE -eq 1 ]; then
      APIMODULE_BUILD_PARAMS="$APIMODULE_BUILD_PARAMS -v"
    fi
    ./make.sh $APIMODULE_BUILD_PARAMS

    cd ../../$BUILD_DIR
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
# Function used to make OSX bundles
# arg1: module name (cgame, qagame or ui)
#
function make_bundle() {
  # Check current OS
  if [ $OS != "Darwin" ]; then
    return
  fi

  echo -n "Making bundle for $1..."
  cd $MOD_NAME
  mkdir $1_mac.bundle
  mkdir $1_mac.bundle/Contents
  mkdir $1_mac.bundle/Contents/MacOS
  cp $1_mac $1_mac.bundle/Contents/MacOS/$1_mac

  echo '<?xml version="1.0" encoding="UTF-8"?>
  <!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
  <plist version="1.0">
  <dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleExecutable</key>
    <string>$1_mac</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>$1</string>
    <key>CFBundlePackageType</key>
    <string>BNDL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.01c</string>
    <key>CFBundleSignature</key>
    <string>JKAm</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFPlugInDynamicRegisterFunction</key>
    <string></string>
    <key>CFPlugInDynamicRegistration</key>
    <string>NO</string>
    <key>CFPlugInFactories</key>
    <dict>
      <key>00000000-0000-0000-0000-000000000000</key>
      <string>MyFactoryFunction</string>
    </dict>
    <key>CFPlugInTypes</key>
    <dict>
      <key>00000000-0000-0000-0000-000000000000</key>
      <array>
        <string>00000000-0000-0000-0000-000000000000</string>
      </array>
    </dict>
    <key>CFPlugInUnloadFunction</key>
    <string></string>
  </dict>
  </plist>' > $1_mac.bundle/Contents/Info.plist

  # Special case for qagame
  # (qagame is not compressed)
  if [ $1 != 'qagame' ]; then
    rm -f $1_mac.zip
    zip -r9 -q $1_mac.zip $1_mac.bundle

    if [ $? -ne 0 ]; then
      echo "An error occured while making bundle for $1"
      exit 1
    fi

    mv $1_mac.zip $1_mac
  fi

  echo "[done]"
  cd ..
}

#
# Make pk3 in $BUILD_DIR
#
function make_pk3() {
  # Copy assets next to game modules
  cp -rf ../$MOD_NAME/* $MOD_NAME
  # Delete previous pk3 if any
  rm -f $PK3_NAME
  cd $MOD_NAME
  zip -9qr ../$PK3_NAME * -x custommapscripts\* qagame\* timeruns.mod
  cd ../..
}

#
# Main
#
parse_options "$@"
clean
detect_os
build
strip_modules
# We don't make bundle anymore as they are not needed by ET: Legacy
#make_bundle qagame
#make_bundle cgame
#make_bundle ui
if [ $MAKE_PK3 -eq 1 ]; then
  make_pk3
fi
