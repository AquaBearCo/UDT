#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#*******************************************************************************
#
# This will build ALL variants!
#
# This expects the following environment variables set:
#
# COMPILE_OS      - linux, macosx, windows
# COMPILE_OS_ARCH - 32, 64, arm
#
#
# possible packages you might need in order to compile:
#  gcc-4.8 gcc-4.8-multilib g++-4.8-multilib gdb make libgtk2.0-dev libxtst-dev libc6-dev-i386 mingw-w64
#  ccache libssl0.9.8
#
# Then, in gcc_apple_amd64 dir: sudo dpkg -i *.deb (this is 64bit gcc to build macosx binaries)
#
# Because we fake the 32bit/arm library calls during compile time (the libs are
# loaded during run time) we can build 32bit/arm builds on a 64bit system without needing multi-arch
# GTK libraries (fyi: there aren't any... and DO NOT install the i386 ones, since they will REMOVE what
# you already have - buggering your system!)
#
#
#
# TODO: clean up below, verify arm builds.
#  # arm:  gcc-arm-linux-gnueabi gcc-4.8-arm-linux-gnueabihf
# hf or no hf? beaglebone, rasbpi, ODROID-U3, other ARM devices
#
# ARMv7sf needs gcc-arm-linux-gnueabi, ARMv7hf needs gcc-arm-linux-gnueabihf
#
# To compile for the ARMv7 (BeagleBone Black), follow:
# http://www.michaelhleonard.com/cross-compile-for-beaglebone-black/
#
# for raspi/BBB
# http://www.michaelhleonard.com/cross-compile-for-beaglebone-black/
# http://derekmolloy.ie/beaglebone/setting-up-eclipse-on-the-beaglebone-for-c-development/
# http://stackoverflow.com/questions/9324772/cross-compiling-static-c-hello-world-for-android-using-arm-linux-gnueabi-gcc?rq=1
#
#
#for ANY OTHER OS/ARCH, the easiest, and most straight forward method is to use
# crosstool-ng. http://crosstool-ng.org/
# howto's:
# http://raspberrypi.stackexchange.com/questions/1/how-do-i-build-a-gcc-4-7-toolchain-for-cross-compiling
# http://www.bootc.net/archives/2012/05/26/how-to-build-a-cross-compiler-for-your-raspberry-pi/
# http://www.ps3devwiki.com/wiki/Cross_Compiling
#
#
######################################################################
# as per: http://stackoverflow.com/questions/8937492/what-is-upxs-best-compression-method
# it is NOT a good idea to compress exe's. WHY? Because they run slower, and HDD space is no longer a concern.
# especially since the executable is ~ 500k.
######################################################################
#
#  Copyright 2015 dorkbox, llc
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
######################################################################

set -euo pipefail


####################################################################
####################################################################
####################################################################
####################################################################
# DONE WITH FUNCTIONS
####################################################################
####################################################################
####################################################################
BUILD_ALL=false
if [ -z "${1:-}" ] || [[ "$1" == "all" ]]; then BUILD_ALL=true; fi


# No args will build all.
if [[ $BUILD_ALL == true ]]; then
  cmake --preset linux-x64-release && cmake --build --preset build-linux-x64-release
  cmake --preset linux-x64-debug && cmake --build --preset build-linux-x64-debug
  cmake --preset linux-aarch64-release && cmake --build --preset build-linux-aarch64-release
  cmake --preset linux-armv7hf-release && cmake --build --preset build-linux-armv7hf-release
  cmake --preset win-x64-release && cmake --build --preset build-win-x64-release || true
  cmake --preset win-x64-debug && cmake --build --preset build-win-x64-debug || true
  cmake --preset win-arm64-release && cmake --build --preset build-win-arm64-release || true
  cmake --preset macos-arm64-release && cmake --build --preset build-macos-arm64-release || true
  cmake --preset macos-arm64-debug && cmake --build --preset build-macos-arm64-debug || true
else
  # LINUX
  if [[ "$1" == "linux" ]]; then
      cmake --preset linux-x64-release && cmake --build --preset build-linux-x64-release
      cmake --preset linux-x64-debug && cmake --build --preset build-linux-x64-debug
  elif [[ "$1" == "linux-aarch64" ]]; then
      cmake --preset linux-aarch64-release && cmake --build --preset build-linux-aarch64-release
  elif [[ "$1" == "linux-armv7hf" ]]; then
      cmake --preset linux-armv7hf-release && cmake --build --preset build-linux-armv7hf-release

  # WINDOWS
  elif [[ "$1" == "windows" ]]; then
      cmake --preset win-x64-release && cmake --build --preset build-win-x64-release || true
      cmake --preset win-x64-debug && cmake --build --preset build-win-x64-debug || true
  elif [[ "$1" == "windows-arm64" ]]; then
      cmake --preset win-arm64-release && cmake --build --preset build-win-arm64-release || true

  # MAC
  elif [[ "$1" == "macosx" ]]; then
      cmake --preset macos-arm64-release && cmake --build --preset build-macos-arm64-release || true
      cmake --preset macos-arm64-debug && cmake --build --preset build-macos-arm64-debug || true

  else
    echo "Please one of the following:"
    echo "   [all] - builds linux x64, aarch64, armv7hf; windows x64/arm64; macOS arm64"
    echo "   linux            - builds Linux x64 (Release/Debug)"
    echo "   linux-aarch64    - builds Linux aarch64 (Release)"
    echo "   linux-armv7hf    - builds Linux armv7 hard-float (Release)"
    echo "   windows          - builds Windows x64 (Release/Debug)"
    echo "   windows-arm64    - builds Windows ARM64 (Release)"
    echo "   macosx           - builds macOS arm64 (Release/Debug)"
    exit 0
  fi
fi

