#pragma once

#include <cstdint>
#include <cstddef>
#include <system_error>
#include "udt.h"

struct sockaddr;

namespace udt { namespace plat {

enum class Family { V4, V6 };
enum class NonBlocking { Yes, No };

struct SockAddr {
  Family family{};
  // Storage is external; minimal wrapper for now
};

class UdpSocket {
public:
  UdpSocket() = default;
  ~UdpSocket() = default;
  UdpSocket(const UdpSocket&) = delete; UdpSocket& operator=(const UdpSocket&) = delete;

  static int create(Family fam, UDPSOCKET& out_fd, std::error_code& ec) noexcept;
  static int close(UDPSOCKET fd, std::error_code& ec) noexcept;
  static int set_nonblocking(UDPSOCKET fd, NonBlocking nb, std::error_code& ec) noexcept;
  static int set_reuseaddr(UDPSOCKET fd, bool on, std::error_code& ec) noexcept;
  static int bind(UDPSOCKET fd, const sockaddr* sa, int len, std::error_code& ec) noexcept;
  static int getsockname(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept;
  static int getpeername(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept;
  static int setsockopt_int(UDPSOCKET fd, int level, int optname, int value, std::error_code& ec) noexcept;
  static int getsockopt_int(UDPSOCKET fd, int level, int optname, int& value, std::error_code& ec) noexcept;
  static int set_reuseport(UDPSOCKET fd, bool on, std::error_code& ec) noexcept; // no-op if unsupported

  // Vectored UDP send/recv. vec points to platform iovec/WSABUF array.
  static int send_vectored(UDPSOCKET fd, const sockaddr* sa, int namelen, void* vec, int veclen, int total_len, std::error_code& ec) noexcept;
  static int recv_vectored(UDPSOCKET fd, sockaddr* sa, int& namelen, void* vec, int veclen, int buf_total, std::error_code& ec) noexcept;

  // Optional helpers (capability-checked internally). Return 0 on success or unsupported; -1 on hard error.
  // Sets DSCP value (0..63). Backend maps to IP_TOS/IPV6_TCLASS as available.
  static int set_dscp(UDPSOCKET fd, int dscp, std::error_code& ec) noexcept;
  // Enable/disable Path MTU Discovery when supported. Best-effort no-op on unsupported platforms.
  static int set_pmtud(UDPSOCKET fd, bool enable, std::error_code& ec) noexcept;
};

}} // namespace udt::plat
