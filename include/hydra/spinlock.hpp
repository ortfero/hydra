#pragma once


#include <atomic>
#include <thread>

#include "config.hpp"


namespace hydra {
  
  
class spinlock {
public:

  spinlock() noexcept = default;
  spinlock(spinlock const&) noexcept = delete;
  spinlock& operator = (spinlock const&) noexcept = delete;


  bool try_lock() noexcept {
    if(flag_.load(std::memory_order_relaxed))
      return false;

    return !flag_.exchange(true, std::memory_order_acquire);
  }


  void unlock() noexcept {
    data_.writer.store(false, std::memory_order_release);
  }
  

  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }


private:

  alignas(config::cacheline_size)
    std::atomic_bool flag_{false};

}; // spinlock

  
}
