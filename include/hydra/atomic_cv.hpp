#pragma once



#include <chrono>
#include <atomic>


#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <Windows.h>

#pragma comment(lib, "synchronization.lib")


#else

#error Unsupported OS

#endif



namespace hydra {


#if defined(_WIN32)


  struct atomic_cv {

    atomic_cv() noexcept = default;
    atomic_cv(atomic_cv const&) = delete;
    atomic_cv& operator = (atomic_cv const&) = delete;

    void notify_one() {
      notified_.fetch_add(1, std::memory_order_relaxed);
      WakeByAddressSingle(&notified_);
    }


    void wait() {
      unsigned expected = 0;
      while (notified_.compare_exchange_weak(expected, 0, std::memory_order_relaxed))
        if (!WaitOnAddress(&notified_, &expected, sizeof(notified_), INFINITE))
          return;
      notified_.fetch_sub(1, std::memory_order_relaxed);
    }


    bool wait(std::chrono::milliseconds timeout) {
      unsigned expected = 0;
      while(notified_.compare_exchange_weak(expected, 0, std::memory_order_relaxed))
        if(!WaitOnAddress(&notified_, &expected, sizeof(notified_), DWORD(timeout.count())))
          return false;
      notified_.fetch_sub(1, std::memory_order_relaxed);
      return true;
    }


  private:

    std::atomic_uint notified_{0};
  }; // atomic_cv

#else

#error Unsupported OS

#endif



} // hydra
