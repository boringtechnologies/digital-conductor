#!/bin/sh

CMAKE_KEY=${1}
CMAKE_VALUE=${2}
CMAKE_TYPE=${3:-PATH}

echo "set(${CMAKE_KEY} \"${CMAKE_VALUE}\" CACHE ${CMAKE_TYPE} \"\")" >> CMakeCache.txt