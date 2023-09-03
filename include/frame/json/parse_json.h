#pragma once

#include <google/protobuf/util/json_util.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace frame::proto {

	template <typename T>
	T LoadProtoFromJson(const std::string& json) {
		T proto{};
		google::protobuf::util::JsonParseOptions options{};
		options.ignore_unknown_fields = false;
		auto status =
			google::protobuf::util::JsonStringToMessage(json, &proto, options);
		if (!status.ok()) {
			throw std::runtime_error(
				"Couldn't parse json status error: " +
				status.message().as_string());
		}
		return proto;
	}

	template <typename T>
	T LoadProtoFromJsonFile(const std::filesystem::path& filename) {
		// Empty case (no such file return an empty structure).
		if (filename.empty()) return T{};
		// Try to open it.
		std::ifstream ifs(filename.string(), std::ios::in);
		if (!ifs.is_open()) {
			throw std::runtime_error(
				"Couldn't open file: " + filename.string());
		}
		std::string contents(std::istreambuf_iterator<char>(ifs), {});
		return LoadProtoFromJson<T>(contents);
	}

}  // End namespace frame::proto.
