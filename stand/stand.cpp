#include <cstdio>

#include <hydra/activity.hpp>


int main(int, char**) {

    hydra::activity<int> activity;
    activity.run([](auto& batch){
    });

    std::fprintf(stdout, "Press ENTER to stop...\n");
    std::getchar();

    return 0;
}
