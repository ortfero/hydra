#pragma once


#include <chrono>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <thread>
#include <memory>
#include "sequence.hpp"


namespace hydra {


  template<typename T>
  struct mpsc_queue {

    using size_type = sequence::value_type;
    using value_type = T;

    static constexpr size_type cacheline = 64;


    mpsc_queue() noexcept = default;
    mpsc_queue(mpsc_queue const&) = delete;
    mpsc_queue& operator = (mpsc_queue const&) = delete;
    mpsc_queue(size_type capacity) { reserve(capacity); }
    explicit operator bool () noexcept { return !!pool_; }
    size_type capacity() const noexcept { return capacity_; }


    mpsc_queue(mpsc_queue&& other) noexcept:
      capacity_{other.capacity_}, index_mask_{other.index_mask_},
      pool_{std::move(other.pool_)}, published_{std::move(other.published_)},
      producer_{other.producer_.load(std::memory_order_relaxed)},
      consumer_{other.consumer_} {
      other.capacity_ = 0;
      other.producer_.store(0, std::memory_order_relaxed);
      other.consumer_ = 0;
    }


    mpsc_queue& operator = (mpsc_queue&& other) noexcept {
      capacity_ = other.capacity_; other.capacity_ = 0;
      index_mask_ = other.index_mask_;
      pool_ = std::move(other.pool_);
      published_ = std::move(other.published_);
      producer_.store(other.producer_.load(std::memory_order_relaxed), std::memory_order_relaxed);
      other.producer_.store(0, std::memory_order_relaxed);
      consumer_ = other.consumer_; other.consumer_ = 0;
      return *this;
    }


    void reserve(size_type capacity) {
      capacity = nearest_power_of_2(capacity);      
      published_ = std::make_unique<std::atomic<size_type>[]>(capacity);
      for(size_type n = 0; n != capacity; ++n)
        published_[n] = 0;
      capacity_ = capacity;
      index_mask_ = capacity - 1;
      pool_ = std::make_unique<T[]>(capacity);
    }
    
    
    size_type blocks_count() const noexcept {
      return blocks_count_.load(std::memory_order_relaxed);
    }
    
    
    void clear_blocks_count() noexcept {
      blocks_count_.store(0, std::memory_order_relaxed);
    }


    size_type size() const noexcept {
      return producer_.load(std::memory_order_relaxed) - consumer_;
    }


    T& operator [] (sequence n) noexcept {
      return pool_[n.value() & index_mask_];
    }


    T const& operator [] (sequence n) const noexcept {
      return pool_[n.value() & index_mask_];
    }


    sequence claim() noexcept {

      if(!pool_)
        return sequence{};

      sequence const p{producer_.fetch_add(1, std::memory_order_relaxed)};
      if(p.value() - consumer_ < capacity_)
        return p;

      blocks_count_.fetch_add(1, std::memory_order_relaxed);

      while(p.value() - consumer_ >= capacity_)
        std::this_thread::yield();

      return p;
    }


    template<typename Rep, typename Period>
    sequence claim_for(std::chrono::duration<Rep, Period> const& duration) noexcept {

      if(!pool_)
        return sequence{};

      sequence const p{producer_.fetch_add(1, std::memory_order_relaxed)};

      if(p.value() - consumer_ < capacity_)
        return p;

      blocks_count_.fetch_add(1, std::memory_order_relaxed);

      auto const started = std::chrono::steady_clock::now();

      while(p.value() - consumer_ >= capacity_) {

        std::this_thread::yield();

        if(std::chrono::steady_clock::now() - started >= duration)
          return sequence{};
      }

      return p;
    }


    void publish(sequence n) noexcept {
      published_[n.value() & index_mask_] = n.value() + 1;
    }


    sequence try_fetch() noexcept {
      if(!pool_)
        return sequence{};
      if(published_[consumer_ & index_mask_] != consumer_ + 1)
        return sequence{};
      return sequence{consumer_};
    }
    
    
    void fetched() noexcept {
      ++consumer_;
    }


  private:

    size_type capacity_{0};
    size_type index_mask_{0};
    std::unique_ptr<T[]> pool_;
    std::unique_ptr<std::atomic<sequence::value_type>[]> published_;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // disable 'structure was padded due to alignment specifier'
#endif
    alignas (cacheline)
      std::atomic<size_type> producer_{0};
    alignas (cacheline)
      size_type consumer_{0};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    std::atomic<size_type> blocks_count_{0};


    static uint64_t nearest_power_of_2(uint64_t n) {
      if(n < 2)
        return 2;
      n--;
      n |= n >> 1;
      n |= n >> 2;
      n |= n >> 4;
      n |= n >> 8;
      n |= n >> 16;
      n |= n >> 32;
      n++;
      return n;
    }

  }; // mspc_queue


} // hydra
