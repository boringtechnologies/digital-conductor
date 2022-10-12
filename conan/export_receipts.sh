#!/bin/sh

receipts_path="conan/receipts"

conan export "${receipts_path}/libjuice" 1.0.5@
conan export "${receipts_path}/gst-plugins-good" 1.19.1@
conan export "${receipts_path}/gst-plugins-bad" 1.19.1@