#!/bin/sh

if [ "$#" != "1" ] ; then
  echo "Incorrect parameter list"
  exit
fi

SO="${1%_mac}"

if [ ! -f ${SO}_mac ] ; then
  echo "Missing ${SO}_mac"
  exit
fi

rm -rf ${SO}_mac.bundle

mkdir ${SO}_mac.bundle
mkdir ${SO}_mac.bundle/Contents
mkdir ${SO}_mac.bundle/Contents/MacOS
cp ${SO}_mac ${SO}_mac.bundle/Contents/MacOS/${SO}_mac

cat << EOF > ${SO}_mac.bundle/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>English</string>
  <key>CFBundleExecutable</key>
  <string>${SO}_mac</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>${SO}</string>
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

#if [ "${SO}" = "qagame" ] ; then
#  echo "Uncompressed ${SO}_mac.bundle created"
#else
  #if [ "${SO}" = "api" ] ; then
    #echo "Uncompressed ${SO}_mac.bundle created"
  #else
    rm -f ${SO}_mac.zip
    zip -r9 ${SO}_mac.zip ${SO}_mac.bundle

    if [ "$?" != "0" ] ; then
      echo "Error - couldn't create zipfile"
    else
      mv ${SO}_mac.zip ${SO}_mac
    fi
  #fi
#fi
