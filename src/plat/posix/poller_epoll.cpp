#ifndef WINDOWS
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include "udt/plat/poller.h"

namespace udt { namespace plat {

static uint32_t to_ep(PollEvent ev) {
  uint32_t m = 0;
  if (static_cast<uint32_t>(ev) & static_cast<uint32_t>(PollEvent::Read)) m |= EPOLLIN;
  if (static_cast<uint32_t>(ev) & static_cast<uint32_t>(PollEvent::Write)) m |= EPOLLOUT;
  return m;
}

static PollEvent from_ep(uint32_t ev) {
  PollEvent r = PollEvent::None;
  if (ev & EPOLLIN) r = r | PollEvent::Read;
  if (ev & EPOLLOUT) r = r | PollEvent::Write;
  if (ev & (EPOLLERR | EPOLLHUP)) r = r | PollEvent::Error;
  return r;
}

int Poller::create(int& out_handle, std::error_code& ec) noexcept {
  int h = ::epoll_create1(EPOLL_CLOEXEC);
  if (h < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  out_handle = h; return 0;
}

int Poller::add(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept {
  epoll_event ee{}; ee.events = to_ep(ev); ee.data.fd = fd;
  if (::epoll_ctl(handle, EPOLL_CTL_ADD, fd, &ee) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int Poller::mod(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept {
  epoll_event ee{}; ee.events = to_ep(ev); ee.data.fd = fd;
  if (::epoll_ctl(handle, EPOLL_CTL_MOD, fd, &ee) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int Poller::del(int handle, int fd, std::error_code& ec) noexcept {
  if (::epoll_ctl(handle, EPOLL_CTL_DEL, fd, nullptr) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

int Poller::wait(int handle, PollFd* fds, int count, int timeout_ms, std::error_code& ec) noexcept {
  if (!fds || count <= 0) { return 0; }
  std::vector<epoll_event> evs(static_cast<size_t>(count));
  int n = ::epoll_wait(handle, evs.data(), count, timeout_ms);
  if (n < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  for (int i = 0; i < n; ++i) {
    fds[i].fd = evs[i].data.fd;
    fds[i].revents = from_ep(evs[i].events);
  }
  return n;
}

int Poller::close(int handle, std::error_code& ec) noexcept {
  if (::close(handle) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

}} // namespace

#endif

