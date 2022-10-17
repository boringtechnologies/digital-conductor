#!/bin/sh

# FIXME convert this to cmake target
# Possible solution is https://github.com/miurahr/cmake-qt-packaging-example

echo rm old package...
rm -rf ../../bin/digitalconductor-demo.dmg

echo build new package...
macdeployqt ../../bin/digitalconductor-demo.app -qmldir=../../src/demo/ -dmg
