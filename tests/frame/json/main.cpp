#include <cstdlib>
#include <gtest/gtest.h>

#include "frame/file/image_stb.h"

int main(int ac, char** av)
{
    if (std::getenv("SDL_VIDEODRIVER") == nullptr)
    {
        setenv("SDL_VIDEODRIVER", "dummy", 0);
    }
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
