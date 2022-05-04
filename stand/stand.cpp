#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include <hydra/activity.hpp>


int main(int, char**) {

    hydra::activity<std::string> activity;
    activity.reserve(2);
    auto const run_result = activity.run([](auto& batch) {
        for(auto seq = batch.try_fetch(); !!seq; seq = batch.try_fetch()) {
            std::fprintf(stdout, "%s\n", batch[seq].data());
            batch.fetched();
        }
    });

    if(!run_result) {
        std::fprintf(stderr, "Unable to start activity\n");
        return -1;
    }
    
    for(int i = 0; i != 4000000; ++i) {
        auto const seq = activity.claim();
        assert(!!seq);
        activity[seq] = std::to_string(i);
        activity.publish(seq);
    }

    std::fprintf(stdout, "Press ENTER to stop...\n");
    std::getchar();

    return 0;
}
