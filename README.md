# digital conductor

simple build flow

## Install dependencies with Conan package manager

All deps except Qt can be installed with `conan`, for this just pass `-DWITH_CONAN=ON`

```
pip3 install conan
sh conan/export_receipts.sh
cmake -S . -B build -DQT_DIR="/usr/local/opt/qt/lib/cmake" -DWITH_CONAN=ON
cmake --build build

```

> NOTE: Install Qt from (official distribution)[https://www.qt.io/download-open-source) to avoid https://github.com/Homebrew/discussions/discussions/2823 issue with macdeployqt

## Manual dependencies install

### install libjuice

```
git clone https://github.com/paullouisageneau/libjuice.git
cd libjuice/
mkdir build
cd build/
cmake ..
make juice-static
sudo make install
sudo cp libjuice-static.a /usr/local/lib/
```

### install gRPC as described https://github.com/grpc/grpc/blob/master/BUILDING.md

```
$ [sudo] apt-get install build-essential autoconf libtool pkg-config
$ [sudo] apt-get install cmake
$ [sudo] apt-get install clang libc++-dev
```

```
$ git clone -b RELEASE_TAG_HERE https://github.com/grpc/grpc
$ cd grpc
$ git submodule update --init
```

```
$ mkdir build && cd build
$ cmake ..
$ make
$ [sudo] make install
```

### Install gstreamer

### Debian/Ubuntu Linux

```sh
$ sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio gstreamer1.0-pulseaudio
```

### macOS

```sh
$ brew install gstreamer gst-plugins-{base,good,bad,ugly} gst-libav
```

### Install Qt 6 to home directory

Online installer, etc...

### build sources

```
$ git clone https://github.com/Boring-App/digital-conductor/.git
$ cd digital-conductor/
$ mkdir build && cd build
$ cmake ..
$ make
```

Done!
