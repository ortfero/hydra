#pragma once


#include <utility>
#include <shared_mutex>
#include "shared_spinlock.hpp"


namespace hydra {
  
  
template<typename T, typename L = shared_spinlock> struct synchronized {

  struct unique_access {

    unique_access() = delete;
    unique_access(unique_access const&) = delete;
    unique_access& operator = (unique_access const&) = delete;

    unique_access(synchronized& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T& operator * () const noexcept { return resource_; }
    T* operator -> () const noexcept { return &resource_; }

  private:

    std::unique_lock<L> guard_;
    T& resource_;

  }; // unique_access


  struct shared_access {

    shared_access() = delete;
    shared_access(shared_access const&) = delete;
    shared_access& operator = (shared_access const&) = delete;

    shared_access(synchronized const& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T const& operator * () const noexcept { return resource_; }
    T const* operator -> () const noexcept { return &resource_; }

  private:

    std::shared_lock<L> guard_;
    T const& resource_;

  }; // shared_access


  synchronized() = default;
  synchronized(synchronized const&) = delete;
  synchronized& operator = (synchronized const&) = delete;
  synchronized(synchronized&&) = default;
  synchronized& operator = (synchronized&&) = default;

  template<typename... Args>
  synchronized(Args&&... args):
    resource_(std::forward<Args>(args)...) { }

private:

  L lock_;
  T resource_;

}; // synchronized

  
} // hydra