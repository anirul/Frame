#pragma once

#include <functional>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include "Frame/LevelInterface.h"
#include "Frame/ProgramInterface.h"
#include "Frame/TextureInterface.h"

namespace frame::opengl {

	// Fill multiple textures from a program.
	//		- level					: structure that contain textures.
	//		- program				: program to be used.
	//		- unform_interface		: interface to update the uniforms.
	void FillProgramMultiTexture(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const std::shared_ptr<UniformInterface> uniform_interface);

	// Fill multiple textures from a program.
	//		- level					: structure that contain textures.
	//		- program				: program to be used.
	//		- unform_interface		: interface to update the uniforms.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureMipmap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const std::shared_ptr<UniformInterface> uniform_interface,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func =
		[](const int, const std::shared_ptr<ProgramInterface>) {});

	// Fill multiple cube map texture from a program.
	//		- level					: structure that contain textures.
	//		- program				: program to be used.
	//		- unform_interface		: interface to update the uniforms.
	void FillProgramMultiTextureCubeMap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const std::shared_ptr<UniformInterface> uniform_interface);

	// Fill multiple cube map texture from a program.
	//		- level					: structure that contain textures.
	//		- program				: program to be used.
	//		- unform_interface		: interface to update the uniforms.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureCubeMapMipmap(
		const std::shared_ptr<LevelInterface> level,
		const std::shared_ptr<ProgramInterface> program,
		const std::shared_ptr<UniformInterface> uniform_interface,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func =
		[](const int, const std::shared_ptr<ProgramInterface>) {});

} // End namespace frame::opengl.
