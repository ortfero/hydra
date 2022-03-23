#pragma once


#include "doctest.h"

#include <hydra/spsc_queue.hpp>


TEST_CASE("spsc_queue::spsc_queue") {
    hydra::spsc_queue<int> target;

    REQUIRE(target.capacity() == 0);
    REQUIRE(!target);
    REQUIRE(target.size() == 0);
}
