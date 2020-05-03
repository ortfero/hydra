#pragma once


#include <cstdint>


namespace hydra {


  struct sequence {

    using value_type = int64_t;

    constexpr sequence() noexcept = default;
    sequence(sequence const&) noexcept = default;
    sequence& operator = (sequence const&) noexcept = default;
    explicit operator bool () const noexcept { return value_ != -1; }
    value_type value() const noexcept { return value_; }
    explicit constexpr sequence(value_type v): value_{v} { }

    bool operator == (sequence const& other) const noexcept {
      return value_ == other.value_;
    }

    bool operator != (sequence const& other) const noexcept {
      return value_ != other.value_;
    }

  private:

    value_type value_{-1};

  }; // sequence


} // hydra
