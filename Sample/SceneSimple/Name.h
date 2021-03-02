#pragma once

#include "Sample/Common/NameInterface.h"

// This is a class to hold global parameters.
class Name : public NameInterface
{
public:
	const std::string GetLevelFileName() const override { return level_file_; }

private:
	std::string level_file_ = "SceneSimple.Level.json";
};
