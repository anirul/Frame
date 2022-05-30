#include "Level.h"

#include "Frame/NodeCamera.h"

namespace frame {

	std::optional<EntityId> Level::GetDefaultStaticMeshQuadId() const
	{
		if (quad_id_)
		{
			return quad_id_;
		}
		return std::nullopt;
	}

    std::optional<EntityId> Level::GetNodeIdFromMeshId(EntityId id) const
    {
		assert(id);
        static std::map<EntityId, EntityId> node_id_mesh_id{};
        auto it = node_id_mesh_id.find(id);
        if (it != node_id_mesh_id.end()) return it->second;
		for (const auto& p : id_scene_node_map_)
		{
			auto local_id = p.second->GetLocalMesh();
			if (p.second->GetLocalMesh() == id)
			{
				node_id_mesh_id.insert({ id, p.first });
				return p.first;
			}
		}
		// auto maybe_name = GetNameFromId(id);
		// auto name = (maybe_name) ? maybe_name.value() : std::string("???");
		// throw std::runtime_error(
		//	std::format("mesh {}-{} has no node connected to it!", id, name));
		return std::nullopt;
    }

	std::optional<EntityId> Level::GetDefaultStaticMeshCubeId() const
	{
		if (cube_id_)
		{
			return cube_id_;
		}
		return std::nullopt;
	}

	std::optional<EntityId> Level::GetIdFromName(const std::string& name) const
	{
		if (name.empty()) 
		{
			logger_->warn("name is empty.");
			return std::nullopt;
		}
		try 
		{
			return name_id_map_.at(name);
		}
		catch (std::out_of_range& ex)
		{
			logger_->warn(ex.what());
			return std::nullopt;
		}
	}

	std::optional<std::string> Level::GetNameFromId(EntityId id) const
	{
		try
		{
			return id_name_map_.at(id);
		}
		catch (std::out_of_range& ex)
		{
			logger_->warn(ex.what());
			return std::nullopt;
		}
	}

	std::optional<EntityId> Level::AddSceneNode(
		std::unique_ptr<NodeInterface>&& scene_node)
	{
		EntityId id = GetSceneNodeNewId();
		std::string name = scene_node->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name)) 
			throw std::runtime_error("Name: " + name + " is already in!");
		string_set_.insert(name);
		id_scene_node_map_.insert({ id, std::move(scene_node) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::NODE });
		return id;
	}

	std::optional<EntityId> Level::AddTexture(
		std::unique_ptr<TextureInterface>&& texture)
	{
		EntityId id = GetTextureNewId();
		std::string name = texture->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		string_set_.insert(name);
		id_texture_map_.insert({ id, std::move(texture) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::TEXTURE });
		return id;
	}

	std::optional<EntityId> Level::AddProgram(
		std::unique_ptr<ProgramInterface>&& program)
	{
		EntityId id = GetProgramNewId();
		std::string name = program->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		id_program_map_.insert({ id, std::move(program) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::PROGRAM });
		return id;
	}

	std::optional<EntityId> Level::AddMaterial(
		std::unique_ptr<MaterialInterface>&& material)
	{
		EntityId id = GetMaterialNewId();
		std::string name = material->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		id_material_map_.insert({ id, std::move(material) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::MATERIAL });
		return id;
	}

	std::optional<EntityId> Level::AddBuffer(
		std::unique_ptr<BufferInterface>&& buffer)
	{
		EntityId id = GetBufferNewId();
		std::string name = buffer->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		id_buffer_map_.insert({ id, std::move(buffer) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::BUFFER });
		return id;
	}

	std::optional<EntityId> Level::AddStaticMesh(
		std::unique_ptr<StaticMeshInterface>&& static_mesh)
	{
		EntityId id = GetStaticMeshNewId();
		std::string name = static_mesh->GetName();
		// CHECKME(anirul): maybe this should return std::nullopt.
		if (string_set_.count(name))
			throw std::runtime_error("Name: " + name + " is already in!");
		id_static_mesh_map_.insert({ id, std::move(static_mesh) });
		id_name_map_.insert({ id, name });
		name_id_map_.insert({ name, id });
		id_enum_map_.insert({ id, EntityTypeEnum::STATIC_MESH });
		return id;
	}

	std::optional<std::vector<frame::EntityId>> Level::GetChildList(
		EntityId id) const
	{
		std::vector<EntityId> list;
		try 
		{
			const auto& node = id_scene_node_map_.at(id);
			// Check who has node as a parent.
			for (const auto& id_node : id_scene_node_map_)
			{
				// In case this is node then add it to the list.
				if (id_node.second->GetParentName() == node->GetName())
				{
					list.push_back(id_node.first);
				}
			}
		}
		catch (std::out_of_range& ex)
		{
			logger_->warn(ex.what());
			return std::nullopt;
		}
		return list;
	}

	std::optional<EntityId> Level::GetParentId(EntityId id) const
	{
		try
		{
			std::string name = id_scene_node_map_.at(id)->GetParentName();
			auto maybe_id = GetIdFromName(name);
			if (!maybe_id) return std::nullopt;
			return maybe_id.value();
		}
		catch (std::out_of_range& ex)
		{
			logger_->warn(ex.what());
			return std::nullopt;
		}
	}

	std::unique_ptr<frame::TextureInterface> Level::ExtractTexture(EntityId id)
	{
		auto ptr = GetTextureFromId(id);
		if (!ptr) return nullptr;
		auto node_texture = id_texture_map_.extract(id);
		auto node_name = id_name_map_.extract(id);
		auto node_id = name_id_map_.extract(node_name.mapped());
		auto node_enum = id_enum_map_.extract(id);
		return std::move(node_texture.mapped());
	}

	frame::CameraInterface* Level::GetDefaultCamera()
	{
		auto maybe_camera_id = GetDefaultCameraId();
		if (!maybe_camera_id)
		{
			logger_->info("Could not get the camera id.");
			return nullptr;
		}
		auto camera_id = maybe_camera_id.value();
		auto node_interface = GetSceneNodeFromId(camera_id);
		if (!node_interface)
		{
			logger_->info("Could not get node interface.");
			return nullptr;
		}
		auto node_camera = dynamic_cast<NodeCamera*>(node_interface);
		if (!node_camera)
		{
			logger_->info("Could not get node camera ptr.");
			return nullptr;
		}
		return node_camera->GetCamera();
	}

} // End namespace frame.
