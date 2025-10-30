// Lightweight sync abstraction over existing udt_pthread_* types
// Allows progressive migration to a unified interface.

#pragma once

#include <cstdint>
#include <chrono>
#include <utility>

#include "udtCommon.h"

namespace udt { namespace plat {

class Mutex {
public:
  Mutex() { CGuard::createMutex(m_); }
  ~Mutex() { CGuard::releaseMutex(m_); }
  Mutex(const Mutex&) = delete; Mutex& operator=(const Mutex&) = delete;
  udt_pthread_mutex_t& native() { return m_; }
private:
  udt_pthread_mutex_t m_{};
};

class LockGuard {
public:
  explicit LockGuard(udt_pthread_mutex_t& m): m_(m) { CGuard::enterCS(m_); }
  explicit LockGuard(Mutex& m): m_(m.native()) { CGuard::enterCS(m_); }
  ~LockGuard() { CGuard::leaveCS(m_); }
  LockGuard(const LockGuard&) = delete; LockGuard& operator=(const LockGuard&) = delete;
private:
  udt_pthread_mutex_t& m_;
};

class CondVar {
public:
  CondVar() { CGuard::createCond(cv_); }
  ~CondVar() { CGuard::releaseCond(cv_); }
  CondVar(const CondVar&) = delete; CondVar& operator=(const CondVar&) = delete;

  void signal() {
#ifdef WINDOWS
    SetEvent(cv_);
#else
    pthread_cond_signal(&cv_);
#endif
  }

  void broadcast() {
#ifdef WINDOWS
    SetEvent(cv_);
#else
    pthread_cond_broadcast(&cv_);
#endif
  }

  template <class Rep, class Period>
  bool wait_for(udt_pthread_mutex_t& m, const std::chrono::duration<Rep, Period>& d) {
#ifdef WINDOWS
    (void)m; // win path does not use mutex handle in this impl
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    return WaitForSingleObject(cv_, static_cast<DWORD>(ms)) == WAIT_OBJECT_0;
#else
    using namespace std::chrono;
    timespec ts{};
    auto now = system_clock::now();
    auto tp = now + d;
    auto secs = time_point_cast<seconds>(tp);
    ts.tv_sec = static_cast<long>(secs.time_since_epoch().count());
    ts.tv_nsec = static_cast<long>(duration_cast<nanoseconds>(tp - secs).count());
    return pthread_cond_timedwait(&cv_, &m, &ts) == 0;
#endif
  }

  void wait(udt_pthread_mutex_t& m) {
#ifdef WINDOWS
    (void)m;
    WaitForSingleObject(cv_, INFINITE);
#else
    pthread_cond_wait(&cv_, &m);
#endif
  }

  udt_pthread_cond_t& native() { return cv_; }

private:
  udt_pthread_cond_t cv_{};
};

// Non-owning helpers for existing udt_pthread_* fields
inline void cond_signal(udt_pthread_cond_t& cv) {
#ifdef WINDOWS
  SetEvent(cv);
#else
  pthread_cond_signal(&cv);
#endif
}

inline void cond_signal(udt_pthread_cond_t* cv) {
  if (!cv) return;
  cond_signal(*cv);
}

inline void cond_broadcast(udt_pthread_cond_t& cv) {
#ifdef WINDOWS
  SetEvent(cv);
#else
  pthread_cond_broadcast(&cv);
#endif
}

inline void cond_broadcast(udt_pthread_cond_t* cv) {
  if (!cv) return;
  cond_broadcast(*cv);
}

inline void cond_wait(udt_pthread_cond_t& cv, udt_pthread_mutex_t& m) {
#ifdef WINDOWS
  (void)m;
  WaitForSingleObject(cv, INFINITE);
#else
  pthread_cond_wait(&cv, &m);
#endif
}

inline void cond_wait(udt_pthread_cond_t* cv, udt_pthread_mutex_t* m) {
  if (!cv || !m) return;
  cond_wait(*cv, *m);
}

// Timed wait helper (non-owning). Returns true on signal, false on timeout.
inline bool cond_timedwait_ms(udt_pthread_cond_t& cv, udt_pthread_mutex_t& m, uint64_t ms) {
#ifdef WINDOWS
  (void)m;
  return WaitForSingleObject(cv, static_cast<DWORD>(ms)) == WAIT_OBJECT_0;
#else
  timespec ts{};
  clock_gettime(CLOCK_REALTIME, &ts);
  uint64_t nsec = static_cast<uint64_t>(ts.tv_nsec) + (ms % 1000) * 1000000ULL;
  ts.tv_sec += static_cast<time_t>(ms / 1000 + nsec / 1000000000ULL);
  ts.tv_nsec = static_cast<long>(nsec % 1000000000ULL);
  return pthread_cond_timedwait(&cv, &m, &ts) == 0;
#endif
}

}} // namespace udt::plat
