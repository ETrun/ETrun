#! /bin/bash

#
# ETrun package script
#

set -euo pipefail

#
# Settings
#
GITHUB_MAIN_REPO=https://github.com/ETrun/ETrun
GITHUB_MAPSCRIPTS_REPO=https://github.com/ETrun/mapscripts
GITHUB_MAPSCRIPTS_BRANCH=master
VERBOSE=0
RELEASE_NAME=ETrun-latest
MOD_CLIENT_FILES=(cgame.mp.i386.so cgame.mp.x86_64.so cgame.mp.aarch64.so cgame_mac cgame_mp_x64.dll cgame_mp_x86.dll ui.mp.i386.so ui.mp.x86_64.so ui.mp.aarch64.so ui_mac ui_mp_x64.dll ui_mp_x86.dll)
MOD_SERVER_FILES=(qagame.mp.i386.so qagame.mp.x86_64.so qagame.mp.aarch64.so qagame_mac qagame_mp_x64.dll qagame_mp_x86.dll)
MOD_ALL_FILES=(${MOD_CLIENT_FILES[@]} ${MOD_SERVER_FILES[@]})
MOD_ASSETS_PATH=../etrun
WGET="wget --quiet"
ZIP="zip"
UNZIP="unzip"

function parse_options() {
  while getopts ":hvn:" opt; do
    case $opt in
      h)
        show_usage
        exit 0
        ;;
      v)
        VERBOSE=1
        ;;
      n)
        RELEASE_NAME=$OPTARG
        ;;
      \?)
        echo "Invalid option: -$OPTARG"
        ;;
    esac
  done
}

function setup() {
  cd dist || exit 1
  rm -rf $RELEASE_NAME
  cp -r ETrun $RELEASE_NAME
}

function debug_print() {
  if [ $VERBOSE -eq 1 ]; then
    echo $@
  fi
}

function copy_files() {
  DEST=$RELEASE_NAME/$1
  shift
  FILES=("$@")
  for f in "${FILES[@]}"; do
    debug_print "Copying $f to $DEST"
    cp ../build/etrun/$f $DEST/
  done
}

function fetch_and_install_custom_mapscripts() {
  REMOTE_FILE="$GITHUB_MAPSCRIPTS_REPO/archive/$GITHUB_MAPSCRIPTS_BRANCH.zip"
  debug_print "Downloading $REMOTE_FILE"
  $WGET $REMOTE_FILE
  debug_print "Unzipping downloaded archive into $RELEASE_NAME/server/custommapscripts"
  $UNZIP $GITHUB_MAPSCRIPTS_BRANCH.zip -d "$RELEASE_NAME/server"
  rm $GITHUB_MAPSCRIPTS_BRANCH.zip
  mv "$RELEASE_NAME/server/mapscripts-$GITHUB_MAPSCRIPTS_BRANCH" "$RELEASE_NAME/server/custommapscripts"
}

function create_pk3() {
  PK3_TEMP_DIRECTORY="$RELEASE_NAME/client/pk3"
  mkdir -p $PK3_TEMP_DIRECTORY
  cp -r $MOD_ASSETS_PATH/* $PK3_TEMP_DIRECTORY
  copy_files "client/pk3" "${MOD_CLIENT_FILES[@]}"
  cd $PK3_TEMP_DIRECTORY
  $ZIP --exclude .DS_Store -r ../$RELEASE_NAME.pk3 .
  cd ../../..
  rm -rf $PK3_TEMP_DIRECTORY
}

function create_final_archive() {
  $ZIP --exclude .DS_Store -r $RELEASE_NAME.zip $RELEASE_NAME/*
  rm -rf $RELEASE_NAME
}

function release() {
  copy_files "server" "${MOD_SERVER_FILES[@]}"
  fetch_and_install_custom_mapscripts
  create_pk3
  create_final_archive
}

function show_usage() {
  echo 'Usage: '`basename $0` ' [options...]'
  echo 'Options:'
  echo '  -h               Show this help'
  echo '  -v               Enable verbose mode'
  echo '  -n               Name of release'
}

#
# Main
#
parse_options "$@"
setup
release
