#pragma once

#include <memory>
#include "Frame/ProgramInterface.h"

namespace frame::opengl::file {

	// Load from a name (something like "Blur")
	std::shared_ptr<ProgramInterface> LoadProgram(
		const std::string& name);
	// Load from 2 file names one for vertex and one for fragment.
	std::shared_ptr<ProgramInterface> LoadProgram(
		const std::string& vertex_file,
		const std::string& fragment_file);

} // End namespace frame::file.
