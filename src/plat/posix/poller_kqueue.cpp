#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include "udt/plat/poller.h"

namespace udt { namespace plat {

static int to_filters(PollEvent ev, std::vector<struct kevent>& out, int fd) {
  out.clear();
  if (static_cast<uint32_t>(ev) & static_cast<uint32_t>(PollEvent::Read)) {
    struct kevent kev{}; EV_SET(&kev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    out.push_back(kev);
  }
  if (static_cast<uint32_t>(ev) & static_cast<uint32_t>(PollEvent::Write)) {
    struct kevent kev{}; EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
    out.push_back(kev);
  }
  return 0;
}

static PollEvent from_filter(const struct kevent& kev) {
  PollEvent r = PollEvent::None;
  if (kev.filter == EVFILT_READ) r = r | PollEvent::Read;
  if (kev.filter == EVFILT_WRITE) r = r | PollEvent::Write;
  if (kev.flags & EV_ERROR) r = r | PollEvent::Error;
  return r;
}

int Poller::create(int& out_handle, std::error_code& ec) noexcept {
  int h = ::kqueue();
  if (h < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  out_handle = h; return 0;
}

int Poller::add(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept {
  std::vector<struct kevent> ch;
  to_filters(ev, ch, fd);
  if (!ch.empty()) {
    if (::kevent(handle, ch.data(), static_cast<int>(ch.size()), NULL, 0, NULL) != 0) {
      ec = std::error_code(errno, std::generic_category()); return -1;
    }
  }
  return 0;
}

int Poller::mod(int handle, int fd, PollEvent ev, std::error_code& ec) noexcept {
  // kqueue add is idempotent for same filter+ident, use same path
  return add(handle, fd, ev, ec);
}

int Poller::del(int handle, int fd, std::error_code& ec) noexcept {
  struct kevent ch[2];
  EV_SET(&ch[0], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  EV_SET(&ch[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  // Ignore errors if not present
  (void)::kevent(handle, ch, 2, NULL, 0, NULL);
  (void)ec; return 0;
}

int Poller::wait(int handle, PollFd* fds, int count, int timeout_ms, std::error_code& ec) noexcept {
  if (!fds || count <= 0) return 0;
  std::vector<struct kevent> evs(static_cast<size_t>(count));
  timespec ts{}; timespec* tsp = NULL;
  if (timeout_ms >= 0) {
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
    tsp = &ts;
  }
  int n = ::kevent(handle, NULL, 0, evs.data(), count, tsp);
  if (n < 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  for (int i = 0; i < n; ++i) {
    fds[i].fd = static_cast<int>(evs[i].ident);
    fds[i].revents = from_filter(evs[i]);
  }
  return n;
}

int Poller::close(int handle, std::error_code& ec) noexcept {
  if (::close(handle) != 0) { ec = std::error_code(errno, std::generic_category()); return -1; }
  return 0;
}

}} // namespace

#endif

