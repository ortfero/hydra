#include <cstdio>

#include <hydra/activity.hpp>


int main(int, char**) {

    hydra::activity<int> activity;
    activity.reserve(8192);
    auto const run_result = activity.run([](auto&) {
    });

    if(!run_result) {
        std::fprintf(stderr, "Unable to start activity\n");
        return -1;
    }

    std::fprintf(stdout, "Press ENTER to stop...\n");
    std::getchar();

    return 0;
}
