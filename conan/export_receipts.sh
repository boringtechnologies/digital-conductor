#!/bin/sh

receipts_path="conan/receipts"

conan export "${receipts_path}/libjuice" 1.0.5@
conan export "${receipts_path}/gstreamer" 1.21.1@
conan export "${receipts_path}/gst-libav" 1.21.1@
conan export "${receipts_path}/gst-plugins-base" 1.21.1@
conan export "${receipts_path}/gst-plugins-good" 1.21.1@
conan export "${receipts_path}/gst-plugins-bad" 1.21.1@
conan export "${receipts_path}/libvpx" 1.11.0@
conan export "${receipts_path}/graphene" 1.10.8@
conan export "${receipts_path}/cairo" 1.17.4@