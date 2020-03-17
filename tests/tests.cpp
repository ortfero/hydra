#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <hydra/hydra.hpp>


struct test {
  std::atomic_bool x;
  unsigned y;
};



SCENARIO("Constructed") {

  std::cout << sizeof (test) << std::endl;

}
