#pragma once

#include "Sample/Common/PathInterface.h"

// This is a class to hold global parameters.
class Name : public PathInterface
{
public:
	const std::string GetLevelFileName() const override { return level_file_; }

private:
	std::string level_file_ = "JapaneseFlag.Level.json"; 
};
