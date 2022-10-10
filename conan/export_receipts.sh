#!/bin/sh

conan_path=$(realpath `dirname $0`)

conan export "${conan_path}/receipts/libjuice" 1.0.5@
conan export "${conan_path}/receipts/gst-plugins-good" 1.19.1@
conan export "${conan_path}/receipts/gst-plugins-bad" 1.19.1@