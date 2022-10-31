# digital conductor

[![build](https://github.com/Boring-App/digital-conductor/actions/workflows/build.yml/badge.svg)](https://github.com/Boring-App/digital-conductor/actions/workflows/build.yml)

- [Install Qt 6](#install-qt-6)
- [Install dependencies with Conan package manager](#install-dependencies-with-conan-package-manager)
- [Manual dependencies install](#manual-dependencies-install)
  * [Install gstreamer](#install-gstreamer)
  * [Debian/Ubuntu Linux](#debian-ubuntu-linux)
  * [macOS](#macos)
- [Build the Project](#build-the-project)

## Install Qt 6

Official or unofficial:

- Official: https://www.qt.io/download-open-source
- Unofficial: https://github.com/miurahr/aqtinstall

## Install dependencies with Conan package manager (Recomended)

All deps except Qt can be installed with `conan`, for this just pass `-DWITH_CONAN=ON` and appropriate `-DWITH_CONAN_PROFILE=$(pwd)/conan/profiles/{intel,linux,mx}`

```
brew install cmake
pip3 install conan
sh conan/export_receipts.sh
cmake -S . -B build -DQT_DIR="/usr/local/opt/qt/lib/cmake" -DWITH_CONAN=ON -DWITH_CONAN_PROFILE=$(pwd)/conan/profiles/linux
cmake --build build
```

> NOTE: Install Qt from (official distribution)[https://www.qt.io/download-open-source) to avoid https://github.com/Homebrew/discussions/discussions/2823 issue with macdeployqt

> NOTE: In case of any isssues building dependencies try run `conan` command directly (check [CI](.github/workflows/build.yml))

## Manual dependencies install

- libjuice
  https://github.com/Boring-App/digital-conductor/blob/21b7c0e32871e37fe2bf5874baf1a7ec6ae1cd38/.github/workflows/build.yml#L119-L125
- gRPC
  https://github.com/Boring-App/digital-conductor/blob/21b7c0e32871e37fe2bf5874baf1a7ec6ae1cd38/.github/workflows/build.yml#L132-L138
- cxxopts
  https://github.com/Boring-App/digital-conductor/blob/21b7c0e32871e37fe2bf5874baf1a7ec6ae1cd38/.github/workflows/build.yml#L146-L149

### Install gstreamer

### Debian/Ubuntu Linux

```sh
$ sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

### macOS

```sh
$ brew install gstreamer gst-plugins-{base,good,bad,ugly} gst-libav
```

## Build the Project

```
$ git clone https://github.com/Boring-App/digital-conductor.git
$ cd digital-conductor/
$ cmake -S . -B build
$ cmake --build build
```

Done!

## Code quality

 - `find src -iname \*.h -o -iname \*.cpp -o -iname \*.hpp -o -iname \*.proto | xargs clang-format -i`
 - `find src -iname \*.cpp -not -iname test/app.cpp | xargs clang-tidy -p compile_commands.json`