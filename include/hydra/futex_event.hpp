// This file is part of hydra library
// Copyright 2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <atomic>
#include <chrono>

#if defined(_WIN32)

#    if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) \
        && !defined(_ARM64_)
#        if defined(_M_IX86)
#            define _X86_
#        elif defined(_M_AMD64)
#            define _AMD64_
#        elif defined(_M_ARM)
#            define _ARM_
#        elif defined(_M_ARM64)
#            define _ARM64_
#        endif
#    endif

#    include <synchapi.h>

#    pragma comment(lib, "synchronization.lib")

#elif defined(__linux__)

#    include <linux/futex.h>
#    include <stdint.h>
#    include <sys/syscall.h>
#    include <sys/time.h>
#    include <unistd.h>

#else

#    error Unsupported system

#endif


namespace hydra {


    class futex_event {
    private:
        std::atomic_flag value_;

    public:
        futex_event() noexcept = default;
        futex_event(futex_event const&) = delete;
        futex_event& operator=(futex_event const&) = delete;


        void clear() noexcept { value_.clear(std::memory_order_relaxed); }


        void notify_one() noexcept {
            value_.test_and_set(std::memory_order_relaxed);
#if defined(_WIN32)
            WakeByAddressSingle(&value_);
#elif defined(__linux__)
            syscall(SYS_futex,
                    &value_,
                    FUTEX_WAKE_PRIVATE,
                    1,
                    nullptr,
                    nullptr,
                    0);
#endif
        }


        void wait() noexcept {
            auto undesired = false;
#if defined(_WIN32)
            WaitOnAddress(&value_, &undesired, sizeof(value_), DWORD(-1));
#elif defined(__linux__)
            syscall(SYS_futex,
                    FUTEX_WAIT_PRIVATE,
                    undesired,
                    nullptr,
                    nullptr,
                    0);
#endif
        }


        template<typename Rep, typename Period>
        void wait(std::chrono::duration<Rep, Period> timeout) noexcept {
            using namespace std::chrono;
            auto undesired = false;
#if defined(_WIN32)
            auto const ms = duration_cast<milliseconds>(timeout);
            WaitOnAddress(&value_,
                          &undesired,
                          sizeof(value_),
                          DWORD(timeout.count()));
#elif defined(__linux__)
            auto const secs = duration_cast<seconds>(timeout);
            auto const ns = duration_cast<nanoseconds>(timeout - secs);
            auto const ts =
                timespec {std::time_t(secs.count()), long(ns.count())};
            syscall(SYS_futex, FUTEX_WAIT_PRIVATE, undesired, &ts, nullptr, 0);
#endif
        }

    };   // futex_event

}   // namespace hydra
