#pragma once

#include <absl/status/status.h>
#include <format>
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <print>
#include <stdexcept>
#include <string>

namespace frame::json
{

template <typename T>
const std::string SaveProtoToJson(const T& proto)
{
    std::string output_str;
    google::protobuf::util::JsonPrintOptions json_print_options;
    json_print_options.add_whitespace = true;
    json_print_options.preserve_proto_field_names = true;
    absl::Status status = google::protobuf::util::MessageToJsonString(
        proto, &output_str, json_print_options);
    if (status.ok())
    {
        return output_str;
    }
    throw std::runtime_error(
        std::format("Couldn't serialize proto to string: {}", status.message()));
}

template <typename T>
void SaveProtoToJsonFile(const T& proto, const std::filesystem::path& filename)
{
    if (filename.empty())
    {
        throw std::runtime_error("No filename provided?");
    }
    const std::string& json_str = SaveProtoToJson(proto);
    std::ofstream ofs(filename.c_str());
    if (ofs)
    {
        ofs << json_str;
        ofs.close();
    }
    else
    {
        throw std::runtime_error(
            std::format("Couldn't output to a file: {}", filename.string()));
    }
}

} // End namespace frame::json.
