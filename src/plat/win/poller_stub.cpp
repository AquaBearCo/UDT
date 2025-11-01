#ifdef WINDOWS
#include <windows.h>
#include <winsock2.h>
#include <system_error>
#include "udt/plat/poller.h"

namespace udt { namespace plat {

int Poller::create(int& out_handle, std::error_code& ec) noexcept { out_handle = -1; (void)ec; return 0; }
int Poller::add(int, int, PollEvent, std::error_code&) noexcept { return 0; }
int Poller::mod(int, int, PollEvent, std::error_code&) noexcept { return 0; }
int Poller::del(int, int, std::error_code&) noexcept { return 0; }
int Poller::wait(int, PollFd*, int, int, std::error_code&) noexcept { return 0; }
int Poller::close(int, std::error_code&) noexcept { return 0; }

}} // namespace

#endif

