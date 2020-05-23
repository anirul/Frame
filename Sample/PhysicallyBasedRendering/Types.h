#pragma once

namespace types {

	// List of draw model available.
	enum class draw_model_enum {
		SPHERE = 0,
		APPLE = 1,
		CUBE = 2,
		TORUS = 3,
		MONKEY = 4,
	};

	// Used to search from string to draw model.
	const std::map<std::string, draw_model_enum> shape_draw_model_map =
	{
		{ "Sphere", draw_model_enum::SPHERE },
		{ "Apple",  draw_model_enum::APPLE },
		{ "Cube",   draw_model_enum::CUBE },
		{ "Torus",  draw_model_enum::TORUS },
		{ "Monkey", draw_model_enum::MONKEY },
	};

	// List of texture model available.
	enum class texture_model_enum {
		METAL = 0,
		APPLE = 1,
		PLANKS = 2,
	};

	// Used to search from string to texture model.
	const std::map<std::string, texture_model_enum> name_texture_model_map =
	{
		{ "Metal",	texture_model_enum::METAL },
		{ "Apple",	texture_model_enum::APPLE },
		{ "Planks", texture_model_enum::PLANKS },
	};

	const std::map<draw_model_enum, std::string> draw_model_shape_map = {
		{ draw_model_enum::SPHERE, "Sphere" },
		{ draw_model_enum::APPLE,  "Apple" },
		{ draw_model_enum::CUBE,   "Cube" },
		{ draw_model_enum::TORUS,  "Torus" },
		{ draw_model_enum::MONKEY, "Monkey" },
	};

	const std::map<texture_model_enum, std::string> 
		texture_model_texture_map = {
		{ texture_model_enum::METAL,  "Metal" },
		{ texture_model_enum::APPLE,  "Apple" },
		{ texture_model_enum::PLANKS, "Planks" },
	};

} // End namespace types.