#pragma once


#include <atomic>
#include <thread>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include "config.hpp"


namespace hydra {
  
  
class shared_spinlock {
public:

  shared_spinlock() noexcept = default;
  shared_spinlock(shared_spinlock const&) noexcept = delete;
  shared_spinlock& operator = (shared_spinlock const&) noexcept = delete;


  bool try_lock() noexcept {
    if(data_.writer.load(std::memory_order_relaxed))
      return false;

    if(data_.writer.exchange(true, std::memory_order_acquire))
      return false;

    while(data_.readers.load(std::memory_order_relaxed) > 0)
      relax();

    return true;
  }


  void unlock() noexcept {
    data_.writer.store(false, std::memory_order_release);
  }
  

  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }


  bool try_lock_shared() noexcept {
    if (data_.writer.load(std::memory_order_relaxed))
      return false;

    data_.readers.fetch_add(1, std::memory_order_relaxed);

    bool expected = true;
    if(data_.writer.compare_exchange_weak(expected, true, std::memory_order_relaxed)) {
      data_.readers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

    return true;
  }


  void unlock_shared() noexcept {
    data_.readers.fetch_sub(1, std::memory_order_relaxed);
  }


  void lock_shared() noexcept {
    while(!try_lock_shared())
      std::this_thread::yield();
  }

private:

  alignas(config::cacheline_size) struct data {
    std::atomic_bool writer{false};
    std::atomic_uint readers{0};
  } data_;


  static void relax() {
    #if defined(_MSC_VER)

    #if defined(_M_AMD64) || defined(_M_IX86)
      _mm_pause();
    #elif defined(_M_ARM)
      __yield();
    #endif // _M_IX86

    #else

    #if defined(__x86_64__) || defined(__i386__)
      __asm__ __volatile__ ("pause");
    #elif defined(__arm__)
      __asm__ __volatile__ ("yield");
    #endif // __i386__

    #endif // _MSC_VER
  }
}; // shared_spinlock

  
} // hydra