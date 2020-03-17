#pragma once



#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>



namespace hydra {



struct config {
  static constexpr size_t cashline_size = 64;
};



struct spinlock {

  spinlock() noexcept = default;
  spinlock(spinlock const&) noexcept = delete;
  spinlock& operator = (spinlock const&) noexcept = delete;


  bool try_lock() noexcept {
    if(flag_.load(std::memory_order_relaxed))
      return false;
    return !flag_.exchange(true, std::memory_order_acquire);
  }


  void unlock() noexcept {
    flag_.store(false, std::memory_order_release);
  }

  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }


private:

  alignas(config::cashline_size) std::atomic_bool flag_{false};

}; // spinlock



struct shared_spinlock {

  shared_spinlock() noexcept = default;
  shared_spinlock(spinlock const&) noexcept = delete;
  shared_spinlock& operator = (spinlock const&) noexcept = delete;



  bool try_lock() noexcept {

    if(data_.writer.load(std::memory_order_relaxed))
      return false;

    if(data_.writer.exchange(true, std::memory_order_acquire))
      return false;

    while(data_.readers.load(std::memory_order_acquire) > 0)
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

    ++data_.readers;

    if(data_.writer.load(std::memory_order_relaxed) ||
       data_.writer.load(std::memory_order_acquire)) {
      --data_.readers;
      return false;
    }

    return true;
  }



  void unlock_shared() noexcept {
    --data_.readers;
  }



  void lock_shared() noexcept {
    while(!try_lock_shared())
      std::this_thread::yield();
  }

private:

  alignas(config::cashline_size) struct data {
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



template<typename T, typename L = shared_spinlock> struct synchronized {

  struct unique {

    unique() = delete;
    unique(unique const&) = delete;
    unique& operator = (unique const&) = delete;

    unique(synchronized& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T& operator * () const noexcept { return resource_; }
    T* operator -> () const noexcept { return &resource_; }

  private:

    std::unique_lock<L> guard_;
    T& resource_;

  }; // unique



  struct shared {

    shared() = delete;
    shared(shared const&) = delete;
    shared& operator = (shared const&) = delete;

    shared(synchronized const& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T const& operator * () const noexcept { return resource_; }
    T const* operator -> () const noexcept { return &resource_; }

  private:

    std::shared_lock<L> guard_;
    T const& resource_;

  }; // shared



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
