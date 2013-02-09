#!/bin/bash

BUILD_DIR="build/etrun"

if [ "$#" != "1" ] ; then
  echo "Incorrect parameter list"
  exit
fi

FULL_PATH=$1
MODULE_NAME=${FULL_PATH##*/}
MODULE_NAME=${MODULE_NAME%_*}

if [ ! -f ${FULL_PATH} ] ; then
  echo "Missing ${MODULE_NAME}_mac"
  exit
fi

echo -n "Making an OSX bundle for $MODULE_NAME..."

rm -rf $BUILD_DIR/${MODULE_NAME}_mac.bundle

mkdir $BUILD_DIR/${MODULE_NAME}_mac.bundle
mkdir $BUILD_DIR/${MODULE_NAME}_mac.bundle/Contents
mkdir $BUILD_DIR/${MODULE_NAME}_mac.bundle/Contents/MacOS
cp ${FULL_PATH} $BUILD_DIR/${MODULE_NAME}_mac.bundle/Contents/MacOS/${MODULE_NAME}_mac

cat << EOF > $BUILD_DIR/${MODULE_NAME}_mac.bundle/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>English</string>
  <key>CFBundleExecutable</key>
  <string>${MODULE_NAME}_mac</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>${MODULE_NAME}</string>
  <key>CFBundlePackageType</key>
  <string>BNDL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.01c</string>
  <key>CFBundleSignature</key>
  <string>JKAm</string>
  <key>CFBundleVersion</key>
  <string>1.01c</string>
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
</plist>
EOF

rm -f $BUILD_DIR/${MODULE_NAME}_mac.zip
zip -r9 -q $BUILD_DIR/${MODULE_NAME}_mac.zip $BUILD_DIR/${MODULE_NAME}_mac.bundle

if [ "$?" != "0" ] ; then
  echo "error: couldn't create zipfile"
else
  mv $BUILD_DIR/${MODULE_NAME}_mac.zip $BUILD_DIR/${MODULE_NAME}_mac
  echo "done"
fi
