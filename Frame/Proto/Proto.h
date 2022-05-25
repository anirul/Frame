#pragma once
// This is just an helpfull header that include all the protos.

#include <fstream>
#include <fmt/core.h>
// This is there to avoid most of the warnings.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 4244)
#pragma warning(disable: 4251)
#pragma warning(disable: 4996)
#endif
#include "Level.pb.h"
#include "Material.pb.h"
#include "Math.pb.h"
#include "Pixel.pb.h"
#include "Program.pb.h"
#include "Scene.pb.h"
#include "Size.pb.h"
#include "Texture.pb.h"
#include "Uniform.pb.h"
// Include the json parser.
#include <google/protobuf/util/json_util.h>
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop)
#endif

namespace frame::proto {

	template <typename T>
	T LoadProtoFromJson(const std::string& json)
	{
		T proto{};
		google::protobuf::util::JsonParseOptions options{};
		options.ignore_unknown_fields = false;
		auto status = google::protobuf::util::JsonStringToMessage(
			json,
			&proto,
			options);
		if (!status.ok())
		{
			throw std::runtime_error(
				fmt::format(
					"Couldn't parse json status error: [{}]",
					status.message().as_string()));
		}
		return proto;
	}

	template <typename T>
	T LoadProtoFromJsonFile(const std::string& file_name)
	{
		// Empty case (no such file return an empty structure).
		if (file_name.empty()) return T{};
		// Try to open it.
		std::ifstream ifs_level(file_name.c_str(), std::ios::in);
		if (!ifs_level.is_open())
			throw std::runtime_error("Couldn't open file: " + file_name);
		std::string contents(std::istreambuf_iterator<char>(ifs_level), {});
		return LoadProtoFromJson<T>(contents);
	}

} // End namespace frame::proto.
