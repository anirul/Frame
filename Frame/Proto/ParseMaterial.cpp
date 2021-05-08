#include "ParseMaterial.h"
#include "Frame/Frame.h"
#include "Frame/OpenGL/Material.h"

namespace frame::proto {

	std::shared_ptr<frame::MaterialInterface> ParseMaterialOpenGL(
		const frame::proto::Material& proto_material, 
		LevelInterface* level)
	{
		const std::size_t texture_size = proto_material.texture_names().size();
		const std::size_t inner_size = proto_material.inner_names().size();
		if (texture_size != inner_size)
		{
			throw std::runtime_error(
				fmt::format(
					"Not the same size for texture and inner names: {} != {}",
					texture_size,
					inner_size));
		}
		auto material = std::make_shared<frame::opengl::Material>();
		if (proto_material.program_name().empty())
		{
			throw std::runtime_error(
				fmt::format("No program name in {}.", proto_material.name()));
		}
		const EntityId program_id =
			level->GetIdFromName(proto_material.program_name());
		material->SetProgramId(program_id);
		for (int i = 0; i < inner_size; ++i)
		{
			const EntityId texture_id = 
				level->GetIdFromName(proto_material.texture_names().at(i));
			material->AddTextureId(
				texture_id, 
				proto_material.inner_names().at(i));
		}
		return material;
	}

} // End namespace frame::proto.
