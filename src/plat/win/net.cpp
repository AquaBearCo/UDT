#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "udt/plat/net.h"

namespace udt { namespace plat {

static int to_family(Family fam) { return fam == Family::V4 ? AF_INET : AF_INET6; }

int UdpSocket::create(Family fam, UDPSOCKET& out_fd, std::error_code& ec) noexcept {
  SOCKET s = ::WSASocketW(to_family(fam), SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
  if (s == INVALID_SOCKET) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  out_fd = s;
  return 0;
}

int UdpSocket::close(UDPSOCKET fd, std::error_code& ec) noexcept {
  SOCKET s = fd;
  if (::closesocket(s) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return 0;
}

int UdpSocket::set_nonblocking(UDPSOCKET fd, NonBlocking nb, std::error_code& ec) noexcept {
  SOCKET s = fd;
  u_long mode = (nb == NonBlocking::Yes) ? 1UL : 0UL;
  if (ioctlsocket(s, FIONBIO, &mode) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return 0;
}

int UdpSocket::set_reuseaddr(UDPSOCKET fd, bool on, std::error_code& ec) noexcept {
  SOCKET s = fd;
  BOOL val = on ? TRUE : FALSE;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&val), sizeof(val)) != 0) {
    ec = std::error_code(WSAGetLastError(), std::system_category()); return -1;
  }
  return 0;
}

int UdpSocket::bind(UDPSOCKET fd, const sockaddr* sa, int len, std::error_code& ec) noexcept {
  SOCKET s = fd;
  if (::bind(s, sa, static_cast<int>(len)) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return 0;
}

int UdpSocket::getsockname(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept {
  int l = len;
  if (::getsockname(fd, sa, &l) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  len = l; return 0;
}

int UdpSocket::getpeername(UDPSOCKET fd, sockaddr* sa, int& len, std::error_code& ec) noexcept {
  int l = len;
  if (::getpeername(fd, sa, &l) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  len = l; return 0;
}

int UdpSocket::setsockopt_int(UDPSOCKET fd, int level, int optname, int value, std::error_code& ec) noexcept {
  if (::setsockopt(fd, level, optname, reinterpret_cast<const char*>(&value), sizeof(value)) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return 0;
}

int UdpSocket::getsockopt_int(UDPSOCKET fd, int level, int optname, int& value, std::error_code& ec) noexcept {
  int sz = sizeof(value);
  if (::getsockopt(fd, level, optname, reinterpret_cast<char*>(&value), &sz) != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return 0;
}

int UdpSocket::set_reuseport(UDPSOCKET, bool, std::error_code&) noexcept { return 0; }

int UdpSocket::send_vectored(UDPSOCKET fd, const sockaddr* sa, int namelen, void* vec, int veclen, int total_len, std::error_code& ec) noexcept {
  DWORD sent = 0;
  int res = ::WSASendTo(fd, reinterpret_cast<LPWSABUF>(vec), veclen, &sent, 0, sa, namelen, NULL, NULL);
  if (res != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  return static_cast<int>(sent == 0 ? total_len : sent);
}

int UdpSocket::recv_vectored(UDPSOCKET fd, sockaddr* sa, int& namelen, void* vec, int veclen, int buf_total, std::error_code& ec) noexcept {
  DWORD recvd = 0; DWORD flags = 0; int nl = namelen;
  int res = ::WSARecvFrom(fd, reinterpret_cast<LPWSABUF>(vec), veclen, &recvd, &flags, sa, &nl, NULL, NULL);
  if (res != 0) { ec = std::error_code(WSAGetLastError(), std::system_category()); return -1; }
  namelen = nl; (void)buf_total; return static_cast<int>(recvd);
}

int UdpSocket::set_dscp(UDPSOCKET fd, int dscp, std::error_code& ec) noexcept {
  if (dscp < 0) dscp = 0; if (dscp > 63) dscp = 63;
  int tos = dscp << 2;
  int ok = 0;
#ifdef IP_TOS
  if (setsockopt(fd, IPPROTO_IP, IP_TOS, reinterpret_cast<const char*>(&tos), sizeof(tos)) != 0) {
    ok = -1; ec = std::error_code(WSAGetLastError(), std::system_category());
  }
#endif
#ifdef IPV6_TCLASS
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, reinterpret_cast<const char*>(&tos), sizeof(tos)) != 0) {
    if (ok == 0) { ok = -1; ec = std::error_code(WSAGetLastError(), std::system_category()); }
  } else {
    ok = 0;
  }
#endif
  return ok;
}

int UdpSocket::set_pmtud(UDPSOCKET, bool, std::error_code&) noexcept {
  // No-op on Windows for now; IOCTL/SIO_ options exist but vary by version.
  return 0;
}

int UdpSocket::set_ecn(UDPSOCKET fd, int ecn, std::error_code& ec) noexcept {
  if (ecn < 0) ecn = 0; if (ecn > 3) ecn = 3;
  int ok = 0; int tos = 0; int sz = sizeof(tos);
#ifdef IP_TOS
  if (::getsockopt(fd, IPPROTO_IP, IP_TOS, reinterpret_cast<char*>(&tos), &sz) == 0) {
    tos = (tos & ~0x3) | (ecn & 0x3);
    if (::setsockopt(fd, IPPROTO_IP, IP_TOS, reinterpret_cast<const char*>(&tos), sizeof(tos)) != 0) { ok = -1; ec = std::error_code(WSAGetLastError(), std::system_category()); }
  }
#endif
#ifdef IPV6_TCLASS
  int tclass = 0; int sz6 = sizeof(tclass);
  if (::getsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, reinterpret_cast<char*>(&tclass), &sz6) == 0) {
    tclass = (tclass & ~0x3) | (ecn & 0x3);
    if (::setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, reinterpret_cast<const char*>(&tclass), sizeof(tclass)) != 0) { if (ok == 0) { ok = -1; ec = std::error_code(WSAGetLastError(), std::system_category()); } }
    else { ok = 0; }
  }
#endif
  return ok;
}

}} // namespace

#endif
