# UXDT: Modernized UDT — Breaking the Data Transfer Bottleneck

UDT (protocol) is a reliable UDP based application level data transport protocol for distributed data intensive applications
 over wide area high-speed networks. UDT uses UDP to transfer bulk data with its own reliability control and 
 congestion control mechanisms. The new protocol can transfer data at a much higher speed than TCP does. UDT
  is also a highly configurable framework that can accommodate various congestion control algorithms.  
  - Presentation: [PowerPoint](https://github.com/dorkbox/UDT/blob/master/udt-doc/udt-2009.ppt)
  - Poster: [PDF](https://github.com/dorkbox/UDT/blob/master/udt-doc/udt-sc08-poster.pdf)

### TCP 

TCP is [slow](http://barchart.github.com/barchart-udt/main/presentation-2009/img6.html).
UDT is [fast](http://barchart.github.com/barchart-udt/main/presentation-2009/img9.html).

### UDT

UDT is developed by [Yunhong Gu](http://www.linkedin.com/in/yunhong) and others at University of Illinois and Google.

The original UDT C++ implementation is available under [BSD license](http://udt.sourceforge.net/license.html).
This fork (UXDT) modernizes build, portability, and packaging while retaining the same public API (src/udt.h).

Note on terminology: Throughout this project, the acronym TLS refers to thread-local storage (per-thread data), not Transport Layer Security. Any references to "TLS keys" are about thread-local error storage used internally by the UDT library.


### Key Features

**Fast**. UDT is designed for extremely high speed networks and it has been used to support global data transfer of terabyte sized data sets. UDT is the core technology in many commercial WAN acceleration products.

**Fair and Friendly**. Concurrent UDT flows can share the available bandwidth fairly, while UDT also leaves enough bandwidth for TCP.

**Easy to Use**. UDT resides completely at the application level. Users can simply download the software and start to use it. No kernel reconfiguration is needed. In addition, UDT's API is very similar to the traditional socket API so that existing applications can be easily modified.

**Highly Configurable**. UDT supports user defined congestion control algorithms with a simple configuration. Users may also modify UDT to suit various situations. This feature can also be used by students and researchers to investigate new control algorithms.

**Firewall Friendly**. UDT is completely based on UDP, which makes it easier to traverse the firewall. In addition, multiple UDT flows can share a single UDP port, thus a firewall can open only one UDP port for all UDT connections. UDT also supports rendezvous connection setup.


### Supported Platforms

| ARCH/OS      |  Linux  | Mac OSX | Windows |
|--------------|---------|---------|---------|
| arm-android  |   ???   |         |         |
| arm-rpi      |   ???   |         |         |
| x86/i386     |   YES   |   YES   |   YES   |
| x86-64/amd64 |   YES   |   YES   |   YES   |


### Current Implementation
 - Updates to UDT source 4.11 to fix some misc. CPU timing bugs in Linux (via the sourceforge help forum).
 - Cleaned up source for cross-compile environment in linux
 - Cleaned up preprocessor symbols and removed deprecated
 - Strips unneeded symbols, drastically reducing size
- Static linking to mingw libraries for windows build

## Modernized Build & Packaging (UXDT Fork)

- CMake 3.16+ with target-based configuration (C++17).
- Cross-platform presets for Linux, Windows, macOS; shared and static libs.
- Internal portability layer (UDP sockets and synchronization) with platform backends.
- CI includes Linux sanitizers; macOS and Windows builds covered.
- pkg-config files `udt.pc` and `uxdt.pc` installed alongside CMake package exports.

### Build Quickstart

- Configure + build using presets, examples:
  - Linux x64 Release: `cmake --preset linux-x64-release && cmake --build --preset build-linux-x64-release -j`
  - Linux x64 Debug: `cmake --preset linux-x64-debug && cmake --build --preset build-linux-x64-debug -j`
  - Windows x64 (MSVC): `cmake --preset win-x64-release && cmake --build --preset build-win-x64`
  - macOS arm64: `cmake --preset macos-arm64-release && cmake --build --preset build-macos-arm64`
- Install: `cmake --install build/<preset-dir> --prefix <dest>`

### Options

- `UDT_ENABLE_POLLER` (default ON): build internal Poller abstraction
  - Linux: epoll backend; macOS/BSD: kqueue backend; Windows: stub (IOCP planned)
- `UDT_ENABLE_DOXYGEN` (default OFF): adds `udt-doc` target if Doxygen is found

### pkg-config

The build installs pkg-config files `udt.pc` and `uxdt.pc` for consumers:

```
pkg-config --cflags udt
pkg-config --libs udt
```

### Notes

- TLS in this repository refers to thread-local storage (internal), not Transport Layer Security.
- CMake find_package supports both `udt` and `uxdt`; targets `udt::udt`, `udt::udt_static` (and UXDT aliases) are provided.

