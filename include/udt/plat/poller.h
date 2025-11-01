// Minimal polling abstraction (experimental, internal only)
#pragma once

#include <cstdint>
#include <vector>
#include <system_error>

namespace udt { namespace plat {

enum class PollEvent : uint32_t {
  None = 0,
  Read = 1u << 0,
  Write = 1u << 1,
  Error = 1u << 2,
};

inline PollEvent operator|(PollEvent a, PollEvent b) {
  return static_cast<PollEvent>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool any(PollEvent e) { return static_cast<uint32_t>(e) != 0; }

struct PollFd {
  int fd;
  PollEvent events;
  PollEvent revents{PollEvent::None};
};

class Poller {
public:
  static int create(int& out_handle, std::error_code& ec) noexcept;
  static int add(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept;
  static int mod(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept;
  static int del(int handle, int fd, std::error_code& ec) noexcept;
  // Wait fills revents in fds (size>0). Returns number ready or -1 on error.
  static int wait(int handle, PollFd* fds, int count, int timeout_ms, std::error_code& ec) noexcept;
  static int close(int handle, std::error_code& ec) noexcept;
};

}} // namespace

