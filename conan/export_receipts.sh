#!/bin/sh

conan_path=$(realpath `dirname $0`)

conan export "${conan_path}/receipts/libjuice" 1.0.5@