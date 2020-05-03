#pragma once



#include <chrono>


#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <Windows.h>

#pragma comment(lib, "synchronization.lib")


#else

#error Unsupported OS

#endif



namespace hydra {


#if defined(_WIN32)


  struct atomic_cv {

    atomic_cv() noexcept = default;
    atomic_cv(atomic_cv const&) = delete;
    atomic_cv& operator = (atomic_cv const&) = delete;

    void notify_one() {
      raised_ = true;
      WakeByAddressSingle(&raised_);
    }


    void notify_all() {
      raised_ = true;
      WakeByAddressAll(&raised_);
    }


    void reset() {
      raised_ = false;
    }


    void wait() {
      bool raised = false;
      while(!raised_)
        WaitOnAddress(&raised_, &raised, sizeof(bool), INFINITE);
    }


    bool wait(std::chrono::milliseconds timeout) {
      bool raised = false;
      while(!raised_)
        if(!WaitOnAddress(&raised_, &raised, sizeof(bool), DWORD(timeout.count())))
          return false;
      return true;
    }


  private:

    bool raised_{false};
  }; // atomic_cv

#else

#error Unsupported OS

#endif



} // hydra
