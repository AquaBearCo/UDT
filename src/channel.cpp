// SPDX-License-Identifier: BSD-3-Clause
/*****************************************************************************
Copyright (c) 2001 - 2011, The Board of Trustees of the University of Illinois.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions
  and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the University of Illinois
  nor the names of its contributors may be used to
  endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/

/****************************************************************************
written by
   Yunhong Gu, last updated 01/27/2011
*****************************************************************************/

#ifndef WINDOWS
   #include <netdb.h>
   #include <arpa/inet.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <cstring>
   #include <cstdio>
   #include <cerrno>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include "channel.h"
#include "packet.h"
#include "udt/plat/net.h"

#ifdef WINDOWS
   #define socklen_t int
#endif

#ifndef WINDOWS
   #define NET_ERROR errno
#else
   #define NET_ERROR WSAGetLastError()
#endif


CChannel::CChannel():
m_iIPversion(AF_INET),
m_iSockAddrSize(sizeof(sockaddr_in)),
m_iSocket(),
m_iSndBufSize(65536),
m_iRcvBufSize(65536)
{
}

CChannel::CChannel(int version):
m_iIPversion(version),
m_iSocket(),
m_iSndBufSize(65536),
m_iRcvBufSize(65536)
{
   m_iSockAddrSize = (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

CChannel::~CChannel()
{
}

void CChannel::open(const sockaddr* addr)
{
   // construct a socket via portability layer
   {
      std::error_code ec;
      if (udt::plat::UdpSocket::create(m_iIPversion == AF_INET ? udt::plat::Family::V4 : udt::plat::Family::V6, m_iSocket, ec) != 0)
         throw CUDTException(1, 0, ec.value());
   }

   if (NULL != addr)
   {
      socklen_t namelen = m_iSockAddrSize;
      {
         std::error_code ec;
         // Set reuseaddr/reuseport before bind to improve ephemeral bind behavior
         (void)udt::plat::UdpSocket::set_reuseaddr(m_iSocket, true, ec);
         (void)udt::plat::UdpSocket::set_reuseport(m_iSocket, true, ec);
         if (udt::plat::UdpSocket::bind(m_iSocket, addr, namelen, ec) != 0)
            throw CUDTException(1, 3, ec.value());
      }
   }
   else
   {
      //sendto or WSASendTo will also automatically bind the socket
      addrinfo hints;
      addrinfo* res;

      memset(&hints, 0, sizeof(struct addrinfo));

      hints.ai_flags = AI_PASSIVE;
      hints.ai_family = m_iIPversion;
      hints.ai_socktype = SOCK_DGRAM;

      if (0 != ::getaddrinfo(NULL, "0", &hints, &res))
         throw CUDTException(1, 3, NET_ERROR);

      {
         std::error_code ec;
         // Set reuseaddr/reuseport before bind to improve ephemeral bind behavior
         (void)udt::plat::UdpSocket::set_reuseaddr(m_iSocket, true, ec);
         (void)udt::plat::UdpSocket::set_reuseport(m_iSocket, true, ec);
         if (udt::plat::UdpSocket::bind(m_iSocket, res->ai_addr, static_cast<int>(res->ai_addrlen), ec) != 0)
            throw CUDTException(1, 3, ec.value());
      }

      ::freeaddrinfo(res);
   }

   setUDPSockOpt();
}

void CChannel::open(UDPSOCKET udpsock)
{
   m_iSocket = udpsock;
   setUDPSockOpt();
}

void CChannel::setUDPSockOpt()
{
   #if defined(BSD) || defined(MACOSX)
      // BSD system will fail setsockopt if the requested buffer size exceeds system maximum value
      int maxsize = 64000;
      std::error_code ec1, ec2;
      if (0 != udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_RCVBUF, m_iRcvBufSize, ec1))
         (void)udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_RCVBUF, maxsize, ec1);
      if (0 != udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_SNDBUF, m_iSndBufSize, ec2))
         (void)udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_SNDBUF, maxsize, ec2);
   #else
      // for other systems, if requested is greated than maximum, the maximum value will be automactally used
      {
         std::error_code ecA, ecB;
         if ((0 != udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_RCVBUF, m_iRcvBufSize, ecA)) ||
             (0 != udt::plat::UdpSocket::setsockopt_int(m_iSocket, SOL_SOCKET, SO_SNDBUF, m_iSndBufSize, ecB)))
         throw CUDTException(1, 3, NET_ERROR);
      }
   #endif

   // No receive timeout is set on POSIX; channel is configured nonblocking instead.

   #ifndef WINDOWS
      // Set non-blocking I/O on POSIX
      {
         std::error_code ec;
         if (udt::plat::UdpSocket::set_nonblocking(m_iSocket, udt::plat::NonBlocking::Yes, ec) != 0)
            throw CUDTException(1, 3, ec.value());
      }
   #else
      DWORD ot = 1; //milliseconds
      if (0 != ::setsockopt(m_iSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&ot, sizeof(DWORD)))
         throw CUDTException(1, 3, NET_ERROR);
   #endif
}

void CChannel::close() const
{
   std::error_code ec;
   udt::plat::UdpSocket::close(m_iSocket, ec);
}

int CChannel::getSndBufSize()
{
   int size = 0; std::error_code ec;
   if (0 == udt::plat::UdpSocket::getsockopt_int(m_iSocket, SOL_SOCKET, SO_SNDBUF, size, ec)) m_iSndBufSize = size;
   return m_iSndBufSize;
}

int CChannel::getRcvBufSize()
{
   int size = 0; std::error_code ec;
   if (0 == udt::plat::UdpSocket::getsockopt_int(m_iSocket, SOL_SOCKET, SO_RCVBUF, size, ec)) m_iRcvBufSize = size;
   return m_iRcvBufSize;
}

void CChannel::setSndBufSize(int size)
{
   m_iSndBufSize = size;
}

void CChannel::setRcvBufSize(int size)
{
   m_iRcvBufSize = size;
}

void CChannel::getSockAddr(sockaddr* addr) const
{
   int namelen = m_iSockAddrSize;
   std::error_code ec;
   udt::plat::UdpSocket::getsockname(m_iSocket, addr, namelen, ec);
}

void CChannel::getPeerAddr(sockaddr* addr) const
{
   int namelen = m_iSockAddrSize;
   std::error_code ec;
   udt::plat::UdpSocket::getpeername(m_iSocket, addr, namelen, ec);
}

int CChannel::sendto(const sockaddr* addr, CPacket& packet) const
{
   // convert control information into network order
   if (packet.getFlag())
      for (int i = 0, n = packet.getLength() / 4; i < n; ++ i)
         *((uint32_t *)packet.m_pcData + i) = htonl(*((uint32_t *)packet.m_pcData + i));

   // convert packet header into network order
   //for (int j = 0; j < 4; ++ j)
   //   packet.m_nHeader[j] = htonl(packet.m_nHeader[j]);
   uint32_t* p = packet.m_nHeader;
   for (int j = 0; j < 4; ++ j)
   {
      *p = htonl(*p);
      ++ p;
   }

   int res = 0;
   {
      std::error_code ec;
      const int addrsize = m_iSockAddrSize;
      const int total = CPacket::m_iPktHdrSize + packet.getLength();
      res = udt::plat::UdpSocket::send_vectored(m_iSocket, addr, addrsize, (void*)packet.m_PacketVector, 2, total, ec);
      if (res < 0) res = -1;
   }

   // convert back into local host order
   //for (int k = 0; k < 4; ++ k)
   //   packet.m_nHeader[k] = ntohl(packet.m_nHeader[k]);
   p = packet.m_nHeader;
   for (int k = 0; k < 4; ++ k)
   {
      *p = ntohl(*p);
       ++ p;
   }

   if (packet.getFlag())
   {
      for (int l = 0, n = packet.getLength() / 4; l < n; ++ l)
         *((uint32_t *)packet.m_pcData + l) = ntohl(*((uint32_t *)packet.m_pcData + l));
   }

   return res;
}

int CChannel::recvfrom(sockaddr* addr, CPacket& packet) const
{
   int res = 0;
   {
      std::error_code ec;
      int addrsize = m_iSockAddrSize;
      res = udt::plat::UdpSocket::recv_vectored(m_iSocket, addr, addrsize, (void*)packet.m_PacketVector, 2, CPacket::m_iPktHdrSize + packet.getLength(), ec);
      if (res < 0) res = -1;
   }

   if (res <= 0)
   {
      packet.setLength(-1);
      return -1;
   }

   packet.setLength(res - CPacket::m_iPktHdrSize);

   // convert back into local host order
   //for (int i = 0; i < 4; ++ i)
   //   packet.m_nHeader[i] = ntohl(packet.m_nHeader[i]);
   uint32_t* p = packet.m_nHeader;
   for (int i = 0; i < 4; ++ i)
   {
      *p = ntohl(*p);
      ++ p;
   }

   if (packet.getFlag())
   {
      for (int j = 0, n = packet.getLength() / 4; j < n; ++ j)
         *((uint32_t *)packet.m_pcData + j) = ntohl(*((uint32_t *)packet.m_pcData + j));
   }

   return packet.getLength();
}
