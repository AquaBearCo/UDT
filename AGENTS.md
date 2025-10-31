# UDT Modernization Notes (Agent Guide)

This repository contains a modernized fork of the UDT C++ library with an emphasis on Linux-first support, Windows x64/ARM64 compatibility, and a portability layer to enable future embedded targets. Public API (src/udt.h) remains unchanged to preserve 4.x compatibility.

## Current Status

- Build System
  - CMake 3.16+ target-based configuration; C++17 required
  - Presets for Linux x64/aarch64/armv7hf, Windows x64/ARM64, macOS arm64
  - Shared and static libraries built from a common object target (`udt_objs` with -fPIC)
  - Install/export rules with `udtConfig.cmake` + `udtConfigVersion.cmake`
  - GitHub Actions CI: Linux (native + cross) and Windows x64
  - `build.sh` wraps presets (no 32-bit; macOS arm64 only)

- Portability Layer
  - `include/udt/plat/sync.h`: lightweight mutex/condvar helpers + non-owning wrappers over existing `udt_pthread_*` types
  - `include/udt/plat/net.h`: `UdpSocket` abstraction (create/close/nonblocking/reuseaddr/reuseport/bind, getsockname/getpeername, setsockopt/getsockopt int, vectored send/recv)
  - Implementations: `src/plat/{posix,win}/{sync,net}.cpp`

- Channel Migration
  - `src/channel.cpp` uses `plat::UdpSocket` for: create, bind, nonblocking (POSIX), close
  - Sends/receives via vectored I/O abstraction
  - Buffer options and getsockname/getpeername via net helpers
  - SO_REUSEADDR (and SO_REUSEPORT when available) set before bind

- Code Hygiene
  - Catch polymorphic exceptions by const reference
  - Remove unused variables; suppress unused parameter warnings where appropriate
  - Eliminate deprecated `throw()` on internal signatures
  - README clarifies TLS = thread-local storage (not Transport Layer Security)

- Artifacts
  - Builds `libudt.so` and `libudt.a` (both installable)
  - Aliases: `udt::udt` (shared), `udt::udt_static` (static)

- Platform Policy
  - Windows 32-bit deprecated (warning emitted in CMake)
  - macOS targets arm64 via presets (no Intel)

## Build Quickstart

- Linux x64 (Release/Debug)
  - `cmake --preset linux-x64-release && cmake --build --preset build-linux-x64 -j`
  - `cmake --preset linux-x64-debug && cmake --build --preset build-linux-x64-debug -j`

- Cross (Linux)
  - aarch64: `cmake --preset linux-aarch64-release && cmake --build --preset build-linux-aarch64 -j`
  - armv7hf: `cmake --preset linux-armv7hf-release && cmake --build --preset build-linux-armv7hf -j`

- Windows (MSVC)
  - x64: `cmake --preset win-x64-release && cmake --build --preset build-win-x64`
  - ARM64: `cmake --preset win-arm64-release && cmake --build --preset build-win-arm64`

- macOS (arm64)
  - `cmake --preset macos-arm64-release && cmake --build --preset build-macos-arm64`

- Install
  - `cmake --install build/<preset-dir> --prefix <dest>`

## Terminology

- TLS in this codebase means Thread-Local Storage — not Transport Layer Security. Comments and README reflect this explicitly.

## Constraints & Conventions

- Public API in `src/udt.h` must remain source/ABI-stable for 4.x unless explicitly version-bumped
- Prefer small, reversible changes; avoid wide-scope refactors in a single commit
- Keep platform-specifics contained within `include/udt/plat/*` and `src/plat/*`

## TODO (Near-Term)

- Portability Layer Expansion
  - Migrate remaining direct socket ops in `api.cpp`, `core.cpp`, `queue.cpp` to `plat::UdpSocket`
  - Add optional DSCP/ECN setters and PMTUD toggles (capability-checked)

- Concurrency & Timing
  - Replace ad-hoc timing with `std::chrono` where practical (internal only)
  - Gradually unify pthread/Win32 sync with `plat::sync` helpers (or std::mutex/condvar) in additional hot paths

- Warnings / Cleanups
  - Remove remaining noisy catches by value (sweep if any remain)
  - Continue removing unused variables/parameters, guarded by compiler warnings

- Packaging
  - Add pkg-config file (`udt.pc`) mirroring CMake package metadata
  - Ensure SONAME/versioned symlinks on Linux installs

- CI Enhancements
  - Add ASan/UBSan jobs on Linux Debug
  - Optionally add clang-tidy (modernize/readability/bugprone minimal set)

- Documentation
  - Add Doxygen for public API in `src/udt.h`
  - Update README with example code snippets (socket/create/connect/send/recv)

## TODO (Longer-Term)

- Polling Abstraction
  - Introduce a minimal `Poller` interface and implement epoll/kqueue/IOCP backends (keep epoll default)

- Thread-Local Last Error (Internal)
  - Migrate legacy TLS key usage to C++ `thread_local` in a staged manner; first redirect `setError/getError`, then remove legacy key bookkeeping

- Memory/Ownership
  - Replace raw owning pointers with smart pointers where lifetimes are straightforward (avoid ABI changes)

- Embedded Path (Experimental)
  - Define minimal `plat::net` backend for lwIP/FreeRTOS; build-time option to exclude unsupported features

## Notes for Agents

- Respect public API stability. Any changes to `src/udt.h` require explicit review and versioning discussion
- When touching files, prefer local, surgical diffs and keep consistent style
- Large formatting-only changes are discouraged; rely on functional diffs
- Validate changes with `cmake --build` for at least Linux x64 (Ninja) before proposing commits

