#ifndef WINDOWS
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include "udt/plat/net.h"

namespace udt { namespace plat {

static int to_domain(Family fam) { return fam == Family::V4 ? AF_INET : AF_INET6; }

int UdpSocket::create(Family fam, UDPSOCKET& out_fd, std::error_code& ec) noexcept {
  int fd = ::socket(to_domain(fam), SOCK_DGRAM, 0);
  if (fd < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  out_fd = fd;
  return 0;
}

int UdpSocket::close(UDPSOCKET fd, std::error_code& ec) noexcept {
  if (::close(static_cast<int>(fd)) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int UdpSocket::set_nonblocking(UDPSOCKET fd, NonBlocking nb, std::error_code& ec) noexcept {
  int flags = fcntl(static_cast<int>(fd), F_GETFL, 0);
  if (flags < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  if (nb == NonBlocking::Yes) flags |= O_NONBLOCK; else flags &= ~O_NONBLOCK;
  if (fcntl(static_cast<int>(fd), F_SETFL, flags) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int UdpSocket::set_reuseaddr(UDPSOCKET fd, bool on, std::error_code& ec) noexcept {
  int val = on ? 1 : 0;
  if (::setsockopt(static_cast<int>(fd), SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
    ec = std::error_code(errno, std::generic_category()); return -1;
  }
  return 0;
}

int UdpSocket::bind(UDPSOCKET fd, const sockaddr* sa, int len, std::error_code& ec) noexcept {
  if (::bind(static_cast<int>(fd), sa, static_cast<socklen_t>(len)) != 0) {
    ec = std::error_code(errno, std::generic_category()); return -1;
  }
  return 0;
}

int UdpSocket::getsockname(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept {
  socklen_t sl = static_cast<socklen_t>(len);
  if (::getsockname(static_cast<int>(fd), sa, &sl) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  len = static_cast<int>(sl);
  return 0;
}

int UdpSocket::getpeername(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept {
  socklen_t sl = static_cast<socklen_t>(len);
  if (::getpeername(static_cast<int>(fd), sa, &sl) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  len = static_cast<int>(sl);
  return 0;
}

int UdpSocket::setsockopt_int(UDPSOCKET fd, int level, int optname, int value, std::error_code& ec) noexcept {
  if (::setsockopt(static_cast<int>(fd), level, optname, &value, sizeof(value)) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int UdpSocket::getsockopt_int(UDPSOCKET fd, int level, int optname, int& value, std::error_code& ec) noexcept {
  socklen_t sz = sizeof(value);
  if (::getsockopt(static_cast<int>(fd), level, optname, &value, &sz) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int UdpSocket::set_reuseport(UDPSOCKET fd, bool on, std::error_code& ec) noexcept {
#ifdef SO_REUSEPORT
  int val = on ? 1 : 0;
  if (::setsockopt(static_cast<int>(fd), SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != 0) {
    ec = std::error_code(errno, std::generic_category()); return -1;
  }
#else
  (void)fd; (void)on; (void)ec;
#endif
  return 0;
}

int UdpSocket::send_vectored(UDPSOCKET fd, const sockaddr* sa, int namelen, void* vec, int veclen, int, std::error_code& ec) noexcept {
  msghdr mh{};
  mh.msg_name = const_cast<sockaddr*>(sa);
  mh.msg_namelen = static_cast<socklen_t>(namelen);
  mh.msg_iov = reinterpret_cast<iovec*>(vec);
  mh.msg_iovlen = veclen;
  mh.msg_control = NULL;
  mh.msg_controllen = 0;
  mh.msg_flags = 0;
  int res = ::sendmsg(static_cast<int>(fd), &mh, 0);
  if (res < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return res;
}

int UdpSocket::recv_vectored(UDPSOCKET fd, sockaddr* sa, int& namelen, void* vec, int veclen, int, std::error_code& ec) noexcept {
  msghdr mh{};
  mh.msg_name = sa;
  mh.msg_namelen = static_cast<socklen_t>(namelen);
  mh.msg_iov = reinterpret_cast<iovec*>(vec);
  mh.msg_iovlen = veclen;
  mh.msg_control = NULL;
  mh.msg_controllen = 0;
  mh.msg_flags = 0;
  int res = ::recvmsg(static_cast<int>(fd), &mh, 0);
  if (res < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  namelen = static_cast<int>(mh.msg_namelen);
  return res;
}

int UdpSocket::set_dscp(UDPSOCKET fd, int dscp, std::error_code& ec) noexcept {
  (void)ec;
  if (dscp < 0) dscp = 0; if (dscp > 63) dscp = 63;
  int tos = dscp << 2; // DSCP is upper 6 bits of TOS/Traffic Class
  int ok = 0;
#ifdef IP_TOS
  if (::setsockopt(static_cast<int>(fd), IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) != 0) {
    // keep error, but try IPv6 class as well
    ok = -1; ec = std::error_code(errno, std::generic_category());
  }
#endif
#ifdef IPV6_TCLASS
  if (::setsockopt(static_cast<int>(fd), IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof(tos)) != 0) {
    if (ok == 0) { ok = -1; ec = std::error_code(errno, std::generic_category()); }
  } else {
    ok = 0; // success on v6 path overrides
  }
#endif
  // If neither option exists, treat as no-op success
  return ok;
}

int UdpSocket::set_pmtud(UDPSOCKET fd, bool enable, std::error_code& ec) noexcept {
  (void)ec;
  int ok = 0;
#ifdef IP_MTU_DISCOVER
  int v = enable ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;
  if (::setsockopt(static_cast<int>(fd), IPPROTO_IP, IP_MTU_DISCOVER, &v, sizeof(v)) != 0) {
    ok = -1; ec = std::error_code(errno, std::generic_category());
  }
#endif
#ifdef IPV6_MTU_DISCOVER
  int vv = enable ? IPV6_PMTUDISC_DO : IPV6_PMTUDISC_DONT;
  if (::setsockopt(static_cast<int>(fd), IPPROTO_IPV6, IPV6_MTU_DISCOVER, &vv, sizeof(vv)) != 0) {
    if (ok == 0) { ok = -1; ec = std::error_code(errno, std::generic_category()); }
  } else {
    ok = 0;
  }
#endif
  return ok;
}

int UdpSocket::set_ecn(UDPSOCKET fd, int ecn, std::error_code& ec) noexcept {
  if (ecn < 0) ecn = 0; if (ecn > 3) ecn = 3;
  int ok = 0; int tos = 0;
#ifdef IP_TOS
  socklen_t sz = sizeof(tos);
  if (::getsockopt(static_cast<int>(fd), IPPROTO_IP, IP_TOS, &tos, &sz) == 0) {
    tos = (tos & ~0x3) | (ecn & 0x3);
    if (::setsockopt(static_cast<int>(fd), IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) != 0) { ok = -1; ec = std::error_code(errno, std::generic_category()); }
  }
#endif
#ifdef IPV6_TCLASS
  int tclass = 0; socklen_t sz6 = sizeof(tclass);
  if (::getsockopt(static_cast<int>(fd), IPPROTO_IPV6, IPV6_TCLASS, &tclass, &sz6) == 0) {
    tclass = (tclass & ~0x3) | (ecn & 0x3);
    if (::setsockopt(static_cast<int>(fd), IPPROTO_IPV6, IPV6_TCLASS, &tclass, sizeof(tclass)) != 0) { if (ok == 0) { ok = -1; ec = std::error_code(errno, std::generic_category()); } else {/* keep first error */} }
    else { ok = 0; }
  }
#endif
  return ok;
}

}} // namespace

#endif
