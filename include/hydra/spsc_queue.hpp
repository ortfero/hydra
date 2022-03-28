// This file is part of hydra library
// Copytight (c) 2020-2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-Software-License: MIT

#pragma once


#include <cassert>
#include <chrono>
#include <thread>

#include <hydra/sequence.hpp>


namespace hydra {


    template<typename T>
    class spsc_queue {
    public:
        using size_type = sequence::value_type;
        using value_type = T;

    private:
        using sequence_value = sequence::value_type;

        size_type capacity_ {0};
        sequence_value index_mask_ {0};
        std::unique_ptr<T[]> pool_;
        std::unique_ptr<sequence_value[]> published_;
        sequence_value producer_ {0};
        sequence_value consumer_ {0};
        size_type blocks_count_ {0};

    public:
        spsc_queue() noexcept = default;
        spsc_queue(spsc_queue const&) = delete;
        spsc_queue& operator=(spsc_queue const&) = delete;
        explicit operator bool() noexcept { return !!pool_; }
        size_type blocks_count() const noexcept { return blocks_count_; }
        void clear_blocks_count() noexcept { blocks_count_ = 0; }
        size_type size() const noexcept {
            return size_type(producer_ - consumer_);
        }
        size_type capacity() const noexcept { return capacity_; }


        spsc_queue(spsc_queue&& other) noexcept
            : capacity_ {other.capacity_},
              index_mask_ {other.index_mask_},
              pool_ {std::move(other.pool_)},
              published_ {std::move(other.published_)},
              producer_ {other.producer_},
              consumer_ {other.consumer_} {
            other.capacity_ = 0;
            other.producer_ = 0;
            other.consumer_ = 0;
        }


        spsc_queue& operator=(spsc_queue&& other) noexcept {
            capacity_ = other.capacity_;
            other.capacity_ = 0;
            index_mask_ = other.index_mask_;
            pool_ = std::move(other.pool_);
            published_ = std::move(other.published_);
            producer_ = other.producer_;
            other.producer_ = 0;
            consumer_ = other.consumer_;
            other.consumer_ = 0;
            return *this;
        }


        void reserve(size_type capacity) {
            capacity = nearest_power_of_2(capacity);
            published_ = std::make_unique<size_type[]>(capacity);
            for(size_type n = 0; n != capacity; ++n)
                published_[n] = 0;
            capacity_ = capacity;
            index_mask_ = capacity - 1;
            pool_ = std::make_unique<T[]>(capacity);
        }


        T& operator[](sequence n) noexcept {
            return pool_[n.value() & index_mask_];
        }


        T const& operator[](sequence n) const noexcept {
            return pool_[n.value() & index_mask_];
        }


        sequence claim() noexcept {
            if(!pool_)
                return sequence{};

            sequence const p {producer_++};
            if(p.value() - consumer_ < capacity_)
                return p;

            blocks_count_++;

            while(p.value() - consumer_ >= capacity_)
                std::this_thread::yield();

            return p;
        }


        template<typename Rep, typename Period>
        sequence claim_for(
            std::chrono::duration<Rep, Period> const& duration) noexcept {
                
            if(!pool_)
                return sequence{};

            sequence const p{producer_};
            ++producer_;

            if(p.value() - consumer_ < capacity_)
                return p;

            ++blocks_count_;

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
                return sequence {};

            return sequence{consumer_};
        }


        void fetched() noexcept { ++consumer_; }


    private:
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
    };   // spsc_queue


}   // namespace hydra
