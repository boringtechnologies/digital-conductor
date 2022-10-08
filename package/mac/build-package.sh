#!/bin/sh

echo rm old package...
rm -rf ../../bin/digitalconductor-demo.dmg

echo build new package...
chmod +w ../../bin/digitalconductor-demo.app/Contents/Info.plist
cp  ./Info.plist ../../bin/digitalconductor-demo.app/Contents/
chmod -w ../../bin/digitalconductor-demo.app/Contents/Info.plist
macdeployqt ../../bin/digitalconductor-demo.app -qmldir=../../src/demo/ -dmg
