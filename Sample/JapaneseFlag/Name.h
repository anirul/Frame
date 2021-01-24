#pragma once

#include "Sample/Common/NameInterface.h"

// This is a class to hold global parameters.
class Name : public NameInterface
{
public:
	const std::string& GetGlobalPath() const override { return global_path_; }
	const std::string& GetLevelFileName() const override { return level_file_; }

private:
	std::string global_path_ = "../../../Asset/";
	std::string level_file_ = "Json/JapaneseFlag.Level.json"; 
};
