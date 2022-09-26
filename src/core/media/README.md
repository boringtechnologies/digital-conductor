# Synchronised media playback

## Install dependencies

### Debian/Ubuntu Linux

```sh
$ sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio gstreamer1.0-pulseaudio
```

### macOS

```sh
$ brew install gstreamer gst-plugins-{base,good,bad,ugly} gst-libav
```

## Build

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Run

### Sender

This example starts capture from the first V4L2 camera (you can change the
device path as appropriate).

```sh
### in the build directory
$ ./sndrcv s v /dev/video0
Elements linked successfully
Setting the pipeline to playing
Please provide input
1. Add Receiver
2. Remove Receiver
3. Stop Sender


Running mainloop
1
provide host port rcvrid
0.0.0.0 5000 rx1
```

### Receiver

Sets up a receiver corresponding to the example above.

```sh
$ ./sndrcv r
Using default address 0.0.0.0. please provide udp port
5000
Press 'q' to quit
Setting the pipeline to playing
Running mainloop
```
