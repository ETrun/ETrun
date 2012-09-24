#! /bin/bash

BUILD_DIR=_build

# Parse options
debug=0
build_curl=0
args=`getopt dc $*`
if [ $? != 0 ]
then
  echo 'Usage: ./bootstrap [-d] [-c]'
  exit 2
fi
set -- $args
for i
do
  case "$i"
  in
    -d)
      debug=1
      shift;;

    -c)
      build_curl=1
      shift;;

    --)
      shift; break;;
  esac
done

# Build libcurl & c-ares
	if [ $build_curl -eq 1 ]; then
	cd src/libs/c-ares
	./configure && make
	cd ../curl
	./configure --without-ssl --disable-shared --disable-crypto-auth  --disable-ipv6 --disable-proxy --enable-ares --disable-manual --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --enable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-manual --disable-ipv6 --disable-sspi --disable-cookies --disable-gopher && make
	cd ../../..
fi

# Clean ETrun
make distclean

# Compile for linux
if [ $debug -eq 0 ]; then
	echo "Will now compile *WITHOUT* debug flags"
	sleep 1
	./configure CFLAGS="-Wno-enum-compare -Wno-format-security -Wno-switch -Wno-array-bounds"
else
	echo "Will now compile *WITH* debug flags"
	sleep 1
	./configure CFLAGS="-Wno-enum-compare -Wno-format-security -Wno-switch -Wno-array-bounds -g -ggdb"
fi

make

if [ $? != 0 ]; then
  exit
fi

strip -s src/cgame/.libs/libcgame.so
strip -s src/game/.libs/libgame.so
strip -s src/ui/.libs/libui.so
strip -s src/APImodule/.libs/libapi.so
mv src/cgame/.libs/libcgame.so $BUILD_DIR/cgame.mp.i386.so
mv src/game/.libs/libgame.so $BUILD_DIR/qagame.mp.i386.so
mv src/ui/.libs/libui.so $BUILD_DIR/ui.mp.i386.so
mv src/APImodule/.libs/libapi.so $BUILD_DIR/APImodule.so
