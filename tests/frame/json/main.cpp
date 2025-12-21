#include <cstdlib>
#include <gtest/gtest.h>

#include "frame/file/image_stb.h"

int main(int ac, char** av)
{
    if (std::getenv("SDL_VIDEODRIVER") == nullptr)
    {
#if defined(_WIN32)
        _putenv_s("SDL_VIDEODRIVER", "dummy");
#else
        setenv("SDL_VIDEODRIVER", "dummy", 0);
#endif
    }
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
