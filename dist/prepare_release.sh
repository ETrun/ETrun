#! /bin/bash

#
# ETrun package script
#

set -eo pipefail

#
# Settings
#
GITHUB_MAIN_REPO=https://github.com/ETrun/ETrun
GITHUB_MAPSCRIPTS_REPO=https://github.com/ETrun/mapscripts
GITHUB_MAPSCRIPTS_BRANCH=master
GITHUB_TAG=''
GEOIP_DATABASE_URL=https://geolite.maxmind.com/download/geoip/database/GeoLite2-Country.tar.gz
GEOIP_DATABASE_FILE=GeoLite2-Country.mmdb
DIST_DIR=dist
VERBOSE=0
RELEASE_NAME=ETrun-latest
MOD_CLIENT_FILES=(cgame.mp.i386.so cgame.mp.x64.so cgame_mac cgame_mp_x64.dll cgame_mp_x86.dll ui.mp.i386.so ui.mp.x64.so ui_mac ui_mp_x64.dll ui_mp_x86.dll)
MOD_SERVER_FILES_LINUX=(qagame.mp.i386.so qagame.mp.x64.so)
MOD_SERVER_FILES_MAC=(qagame_mac)
MOD_SERVER_FILES_WIN=(qagame_mp_x64.dll qagame_mp_x86.dll)
MOD_ALL_FILES=(${MOD_CLIENT_FILES[@]} ${MOD_SERVER_FILES_LINUX[@]} ${MOD_SERVER_FILES_MAC[@]} ${MOD_SERVER_FILES_WIN[@]})
MOD_ASSETS_PATH=../etrun
WGET="wget --quiet"
ZIP="zip"
UNZIP="unzip"
UNTAR="tar -xf"

function parse_options() {
  while getopts ":ht:vn:" opt; do
    case $opt in
      h)
        show_usage
        exit 0
        ;;
      t)
        GITHUB_TAG=$OPTARG
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

  if [ -z $GITHUB_TAG ]; then
    echo 'Error: you must provide a GitHub tag, example: -t v1.3.0'
    exit 1;
  fi
}

function setup() {
  rm -rf $RELEASE_NAME
  cp -r ETrun $RELEASE_NAME
}

function debug_print() {
  if [ $VERBOSE -eq 1 ]; then
    echo $@
  fi
}

function fetch_mod_files() {
  for f in "${MOD_ALL_FILES[@]}"
  do
    REMOTE_FILE=$GITHUB_MAIN_REPO/releases/download/$GITHUB_TAG/$f
    if [ ! -f $f ]; then
      debug_print "Downloading $REMOTE_FILE"
      $WGET $REMOTE_FILE
    else
      debug_print "Skipped downloading $f as it already exists locally."
    fi
  done
}

function move_files() {
  DEST=$RELEASE_NAME/$1
  FILES=$2
  for f in "${FILES[@]}"
  do
    debug_print "Moving $f to $DEST"
    mv $f $DEST/
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

function fetch_and_install_geoip_database() {
  ARCHIVE="${GEOIP_DATABASE_URL##*/}"
  $WGET $GEOIP_DATABASE_URL
  $UNTAR $ARCHIVE
  mv GeoLite2-Country_*/$GEOIP_DATABASE_FILE "$RELEASE_NAME/server/"
  rm $ARCHIVE
  rm -rf GeoLite2-Country_*
}

function create_pk3() {
  PK3_TEMP_DIRECTORY="$RELEASE_NAME/client/pk3"
  mkdir -p $PK3_TEMP_DIRECTORY
  cp -r $MOD_ASSETS_PATH/* $PK3_TEMP_DIRECTORY
  move_files "client/pk3" "$(echo ${MOD_CLIENT_FILES[@]})"
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
  fetch_mod_files
  move_files "server/linux" "$(echo ${MOD_SERVER_FILES_LINUX[@]})"
  move_files "server/mac" "$(echo ${MOD_SERVER_FILES_MAC[@]})"
  move_files "server/win" "$(echo ${MOD_SERVER_FILES_WIN[@]})"
  fetch_and_install_custom_mapscripts
  fetch_and_install_geoip_database
  create_pk3
  create_final_archive
}

function show_usage() {
  echo 'Usage: '`basename $0` ' [options...]'
  echo 'Options:'
  echo '  -h               Show this help'
  echo '  -t GITHUB_TAG    GitHub tag to fetch mod files from'
  echo '  -v               Enable verbose mode'
  echo '  -n               Name of release'
}

#
# Main
#
parse_options "$@"
setup
release
