// This is just an helpfull header that include all the protos.

#pragma once

// This is there to avoid most of the warnings.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 4251)
#pragma warning(disable: 4996)
#endif
#include "Display.pb.h"
#include "Effect.pb.h"
#include "Math.pb.h"
#include "Pixel.pb.h"
#include "Scene.pb.h"
#include "Size.pb.h"
#include "Texture.pb.h"
#include "Uniform.pb.h"
// Include the json parser.
#include <google/protobuf/util/json_util.h>
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif
