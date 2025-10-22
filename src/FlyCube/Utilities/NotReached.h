#pragma once

#include <cassert>
#include <cstdlib>

#define NOTREACHED() \
    assert(false);   \
    abort()
