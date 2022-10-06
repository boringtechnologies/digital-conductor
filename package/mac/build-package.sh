#!/bin/sh

echo rm old package...
rm -rf ../../bin/digitalconductor-demo.dmg

echo build new package...
# mv ../../bin/digitalconductor-demo.app/Contents/MacOS/digitalconductor-demo ../../bin/digitalconductor-demo.app/Contents/MacOS/digitalconductor-demo-app
# cp ./digitalconductor-demo ../../bin/digitalconductor-demo.app/Contents/MacOS/
cp -r ./Info.plist ../../bin/digitalconductor-demo.app/Contents/
~/Qt/6.2.4/macos/bin/macdeployqt ../../bin/digitalconductor-demo.app -qmldir=../../src/demo/ -dmg
