#pragma once

// This is there to allow to link with other software that are using STB image.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif
