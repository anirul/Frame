#include <gtest/gtest.h>
#include "CameraTest.h"
#include "ErrorTest.h"
#include "WindowTest.h"

int main(int ac, char** av) 
{
	testing::InitGoogleTest(&ac, av);
	return RUN_ALL_TESTS();
}