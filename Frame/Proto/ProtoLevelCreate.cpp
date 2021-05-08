#include "ProtoLevelCreate.h"

namespace frame::proto {

	Level GetLevel()
	{
		Level level{};
		level.set_name("test");
		level.set_default_texture_name("texture");
		return level;
	}

	ProgramFile GetProgramFile()
	{
		ProgramFile program_file{};
		Program program{};
		{
			program.set_name("program");
			program.set_shader("Blur");
			SceneType scene_type;
			scene_type.set_value(SceneType::QUAD);
			*program.mutable_input_scene_type() = scene_type;
			program.add_output_texture_names("texture");
		}
		*program_file.add_programs() = program;
		return program_file;
	}

	SceneTreeFile GetSceneFile()
	{
		SceneTreeFile scene_tree_file{};
		{
			SceneMatrix scene_matrix{};
			{
				scene_matrix.set_name("root");
			}
			*scene_tree_file.add_scene_matrices() = scene_matrix;
			SceneCamera scene_camera{};
			{
				scene_camera.set_name("camera");
				scene_camera.set_parent("root");
				scene_camera.set_fov_degrees(60.0f);
			}
			*scene_tree_file.add_scene_cameras() = scene_camera;
			scene_tree_file.set_default_root_name("root");
			scene_tree_file.set_default_camera_name("camera");
		}
		return scene_tree_file;
	}

	TextureFile GetTextureFile(const std::string& filename /*= ""*/)
	{
		TextureFile texture_file{};
		Texture texture{};
		if (filename.empty())
		{
			texture.set_name("texture");
			Size size;
			size.set_x(-1);
			size.set_y(-1);
			*texture.mutable_size() = size;
		}
		else
		{
			texture.set_name("texture");
			texture.set_file_name(filename);
		}
		texture.mutable_pixel_element_size()->set_value(
			PixelElementSize::BYTE);
		texture.mutable_pixel_structure()->set_value(PixelStructure::RGB);
		*texture_file.add_textures() = texture;
		return texture_file;
	}

	MaterialFile GetMaterialFile()
	{
		return {};
	}

} // End namespace frame::proto.
