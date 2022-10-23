#pragma once
// This is just an helpfull header that include all the protos.

#include <fmt/core.h>

#include <filesystem>
#include <fstream>

// This is there to avoid most of the warnings.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4005)
#pragma warning(disable : 4244)
#pragma warning(disable : 4251)
#pragma warning(disable : 4996)
#endif
#include "frame/proto/level.pb.h"
#include "frame/proto/material.pb.h"
#include "frame/proto/math.pb.h"
#include "frame/proto/pixel.pb.h"
#include "frame/proto/program.pb.h"
#include "frame/proto/scene.pb.h"
#include "frame/proto/size.pb.h"
#include "frame/proto/texture.pb.h"
#include "frame/proto/uniform.pb.h"
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif

#include "frame/json/parse_json.h"
