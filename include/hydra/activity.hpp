#pragma once


#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <utility>

#include <hydra/batch.hpp>
#include <hydra/mpsc_queue.hpp>
#include <hydra/sequence.hpp>


namespace hydra {


    template<typename M, typename Q = mpsc_queue<M>>
    class activity {
    public:
        using message_type = M;
        using queue_type = Q;
        using size_type = typename Q::size_type;
        using batch_type = batch<Q>;

    private:
        std::thread worker_;
        queue_type messages_;
        std::atomic_uint32_t new_message_;
        std::atomic_flag stopping_ {};

    public:
        activity() noexcept = default;
        activity(activity const&) noexcept = delete;
        activity& operator=(activity const&) noexcept = delete;
        ~activity() { stop(); }
        bool active() const noexcept { return worker_.joinable(); }
        sequence claim() noexcept { return messages_.claim(); }
        message_type& operator[](sequence n) noexcept { return messages_[n]; }
        void reserve(size_type n) noexcept { messages_.reserve(n); }

        
        size_type blocks_count() const noexcept {
            return messages_.blocks_count();
        }


        template<typename Rep, typename Period>
        sequence claim_for(
            std::chrono::duration<Rep, Period> const& duration) noexcept {
            return messages_.claim_for(duration);
        }


        void publish(sequence n) noexcept {
            messages_.publish(n);
            new_message_.fetch_add(1, std::memory_order_relaxed);
            new_message_.notify_one();
        }


        void stop() noexcept {
            auto const is_stopped_or_stopping = !worker_.joinable()
                || stopping_.test(std::memory_order_relaxed);
            if(is_stopped_or_stopping)
                return;
            stopping_.test_and_set(std::memory_order_relaxed);
            new_message_.fetch_add(1, std::memory_order_relaxed);
            new_message_.notify_one();
            worker_.join();
        }


        template<typename H>
        bool run(H&& handler) {
            if(worker_.joinable() || !messages_)
                return false;

            worker_ = std::thread {[handler, this]() {
                while(!stopping_.test(std::memory_order_relaxed)) {
                    new_message_.wait(0);
                    process(handler);
                }
                process(handler);
                new_message_.store(0, std::memory_order_relaxed);
                stopping_.clear(std::memory_order_relaxed);
            }};

            return true;
        }

    private:

        template<typename H>
        void process(H&& handler) {
            while(messages_.size() != 0) {
                auto messages = batch_type{messages_};
                handler(messages);
                new_message_.fetch_sub(messages.fetched_count(), std::memory_order_relaxed);
            }
        }
    };   // activity


}   // namespace hydra
