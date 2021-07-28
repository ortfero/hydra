#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "spinlock.hpp"
#include "shared_spinlock.hpp"
#include "mpsc_queue.hpp"
#include "atomic_cv.hpp"
#include "activity.hpp"
