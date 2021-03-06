#pragma once

#include <string>
#include "Frame/File/FileSystem.h"

struct NameInterface
{
	virtual const std::string GetGlobalPath() const 
	{ 
		return frame::file::FindDirectory("Asset/Json/");
	}
	virtual const std::string GetLevelFileName() const = 0;
};
