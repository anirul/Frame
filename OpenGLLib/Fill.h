#pragma once

#include <glm/glm.hpp>
#include "../OpenGLLib/Texture.h"
#include "../OpenGLLib/Device.h"

namespace frame::opengl {

	// Fill multiple textures from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- in_textures			: input textures (with associated string).
	//		- program				: program to be used.
	void FillProgramMultiTexture(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program);

	// Fill multiple textures from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- in_textures			: input textures (with associated string).
	//		- program				: program to be used.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func =
		[](const int, const std::shared_ptr<ProgramInterface>) {});

	// Fill multiple cube map texture from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- in_textures			: input textures (with associated string).
	//		- program				: program to be used.
	void FillProgramMultiTextureCubeMap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program);

	// Fill multiple cube map texture from a program.
	//		- out_textures			: output textures (should be allocated).
	//		- in_textures			: input textures (with associated string).
	//		- program				: program to be used.
	//		- mipmap				: level of mipmap (0 == 1).
	//		- func					: a lambda that will be call per mipmap.
	void FillProgramMultiTextureCubeMapMipmap(
		std::vector<std::shared_ptr<Texture>>& out_textures,
		const std::map<std::string, std::shared_ptr<Texture>>& in_textures,
		const std::shared_ptr<ProgramInterface> program,
		const int mipmap,
		const std::function<void(
			const int mipmap,
			const std::shared_ptr<ProgramInterface> program)> func =
		[](const int, const std::shared_ptr<ProgramInterface>) {});

} // End namespace frame::opengl.
