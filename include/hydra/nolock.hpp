#pragma once


namespace hydra {
  
  
struct nolock {
  
  nolock() noexcept = default;
  nolock(nolock const&) noexcept = delete;
  nolock& operator = (nolock const&) noexcept = delete;
  
  bool try_lock() noexcept { return true; }
  void unlock() noexcept { }
  void lock() noexcept { }
  
}; // nolock

  
} // hydra
