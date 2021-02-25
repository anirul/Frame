#include "Rendering.h"
#include <stdexcept>
#include <fmt/core.h>
#include "Fill.h"

namespace frame::opengl {

	void Rendering::DisplayTexture(
		std::weak_ptr<ProgramInterface> program,
		const double dt /*= 0.0 */)
	{
		auto program_ptr = program.lock();
		if (!program_ptr)
			throw std::runtime_error("Program weak_ptr doesn't exist.");
		throw std::runtime_error("Not implemented!");
	}

	void Rendering::DisplayCubeMap(
		std::weak_ptr<ProgramInterface> program,
		const double dt /*= 0.0 */)
	{
		auto program_ptr = program.lock();
		if (!program_ptr)
			throw std::runtime_error("Program weak_ptr doesn't exist.");
		throw std::runtime_error("Not implemented!");
	}

	void Rendering::DisplayMesh(
		std::weak_ptr<ProgramInterface> program, 
		std::weak_ptr<StaticMeshInterface> static_mesh, 
		const double dt /*= 0.0*/)
	{
		auto program_ptr = program.lock();
		auto static_mesh_ptr = static_mesh.lock();
		if (!program_ptr)
			throw std::runtime_error("Program weak_ptr doesn't exist.");
		if (!static_mesh_ptr)
			throw std::runtime_error("StatiMesh weak_ptr doesn't exist.");
		throw std::runtime_error("Not implemented!");
	}

} // End namespace frame::opengl.
