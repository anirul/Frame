#include "ParseMaterial.h"
#include "Frame/LevelInterface.h"
#include "Frame/OpenGL/Material.h"

namespace frame::proto {

	std::shared_ptr<frame::MaterialInterface> ParseMaterialOpenGL(
		const frame::proto::Material& proto_material, 
		const std::shared_ptr<LevelInterface> level)
	{
		const std::size_t texture_size = proto_material.texture_names().size();
		const std::size_t inner_size = proto_material.inner_names().size();
		if (texture_size != inner_size)
		{
			throw std::runtime_error(
				"Not the same size for texture and inner names: " +
				std::to_string(texture_size) + " != " +
				std::to_string(inner_size) + ".");
		}
		auto material = std::make_shared<frame::opengl::Material>(level);
		for (int i = 0; i < inner_size; ++i)
		{
			const std::uint64_t id = 
				level->GetIdFromName(proto_material.texture_names().at(i));
			material->AddTextureId(id, proto_material.inner_names().at(i));
		}
		return material;
	}

} // End namespace frame::proto.
