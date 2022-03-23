#pragma once


#include <hydra/sequence.hpp>


namespace hydra {


    template<typename Q>
    class batch {
    public:
        using size_type = typename Q::size_type;
        using value_type = typename Q::value_type;

    private:
        Q& queue_;
        size_type size_;

    public:
        batch() = delete;
        batch(const batch&) = delete;
        batch& operator=(const batch&) = delete;
        batch(Q& queue) noexcept: queue_(queue), size_ {queue.size()} {}

        size_type size() const noexcept { return size_; }
        sequence try_fetch() { return queue_.try_fetch(); }
        void fetched() { queue_.fetched(); }
        value_type& operator[](sequence n) { return queue_[n]; }
    };   // batch


}   // namespace hydra
