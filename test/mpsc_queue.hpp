#pragma once



#include <thread>
#include <future>

#include "doctest.h"

#include <hydra/mpsc_queue.hpp>



TEST_CASE("mpsc_queue::mpsc_queue()") {

  hydra::mpsc_queue<int> target;

  REQUIRE(target.capacity() == 0);
  REQUIRE(!target);
  REQUIRE(target.size() == 0);
}


TEST_CASE("mpsc_queue::mpsc_queue(capacity)") {

  hydra::mpsc_queue<int> target(1);

  REQUIRE(target.capacity() == 2);
  REQUIRE(!!target);
  REQUIRE(target.size() == 0);
}


TEST_CASE("mpsc_queue::mpsc_queue(mpsc_queue&&)") {

  using queue = hydra::mpsc_queue<int>;
  queue target(6);
  queue moved1{std::move(target)};

  REQUIRE(!!moved1);
  REQUIRE(moved1.capacity() == 8);
  REQUIRE(moved1.size() == 0);
  REQUIRE(!target);
  REQUIRE(target.capacity() == 0);
  REQUIRE(target.size() == 0);

  queue moved2;
  moved2 = std::move(moved1);

  REQUIRE(!!moved2);
  REQUIRE(moved2.capacity() == 8);
  REQUIRE(moved2.size() == 0);
  REQUIRE(!moved1);
  REQUIRE(moved1.capacity() == 0);
  REQUIRE(moved1.size() == 0);
}


TEST_CASE("mpsc_queue::claim") {

  hydra::mpsc_queue<int> target(1);
  auto const c1 = target.claim();

  REQUIRE(!!c1);
  REQUIRE(c1.value() == 0);
  REQUIRE(target.size() == 1);

  auto const c2 = target.claim();

  REQUIRE(!!c2);
  REQUIRE(c2.value() == 1);
  REQUIRE(target.size() == 2);

  auto const c3 = target.claim_for(std::chrono::microseconds{1});
  REQUIRE(!c3);
}


TEST_CASE("mpsc_queue::operator []") {

  hydra::mpsc_queue<int> target(1);
  auto const c1 = target.claim();
  target[c1] = -3;
  REQUIRE(target[c1] == -3);
}


TEST_CASE("mpsc_queue::publish") {

  hydra::mpsc_queue<int> target(1);

  auto const n1 = target.claim();
  target[n1] = -3;
  target.publish(n1);

  REQUIRE(target.size() == 1);
}


TEST_CASE("mpsc_queue::try_fetch") {

  hydra::mpsc_queue<int> target(1);

  auto const f1 = target.try_fetch();
  REQUIRE(!f1);

  auto const p1 = target.claim();
  target[p1] = -3;

  auto const f2 = target.try_fetch();
  REQUIRE(!f2);

  target.publish(p1);

  auto const f3 = target.try_fetch();
  REQUIRE(!!f3);
  REQUIRE(f3 == p1);
  target.fetched();
}


TEST_CASE("mpsc_queue::multithreading") {

  hydra::mpsc_queue<int> target(1);
  constexpr auto from_number = 1;
  constexpr auto to_number = 1000;
  constexpr auto numbers_count = to_number - from_number + 1;

  auto summator = std::async(std::launch::async, [&]{
    auto count = 0, sum = 0;
    while(count != numbers_count) {
      auto const p = target.try_fetch();
      if(!p)
        continue;
      sum += target[p];
      target.fetched();
      ++count;
    }
    return sum;
  });

  for(auto n = from_number; n != to_number + 1; ++n) {
    auto const p = target.claim();
    if(!p)
      continue;
    target[p] = n;
    target.publish(p);
  }

  REQUIRE(summator.get() == (from_number + to_number) * numbers_count / 2);
}

