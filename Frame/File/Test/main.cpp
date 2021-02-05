#include <gtest/gtest.h>
#include "FileSystemTest.h"

int main(int ac, char** av)
{
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}