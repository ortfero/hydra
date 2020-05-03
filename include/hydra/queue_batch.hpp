#pragma once


#include "sequence.hpp"


namespace hydra {
  
  
  template<typename Q>
  struct queue_batch {
    
    using size_type = typename Q::size_type;
    using value_type = typename Q::value_type;
    
    
    queue_batch() = delete;    
    queue_batch(const queue_batch&) = delete;
    queue_batch& operator = (const queue_batch&) = delete;
    queue_batch(Q& queue) noexcept: queue_(queue) { }
    
    size_type size() const noexcept { return queue_.size(); }
    sequence try_fetch() { return queue_.try_fetch(); }    
    void fetched() { queue_.fetched(); }
    value_type& operator [] (sequence n) { return queue_[n]; }

    
  private:
  
    Q& queue_;
    
  }; // queue_batch
  
  
} // hydra
