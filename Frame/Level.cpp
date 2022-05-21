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

	void Level::PushBackPreProcess(
		const std::string& name, 
		const std::string& mesh_name,
		const std::string& material_name)
	{
		PushBackSceneProcess(
			name,
			mesh_name,
			material_name,
			pre_process_names_,
			pre_process_mesh_ids_,
			pre_process_material_ids_);
	}

    void Level::PushBackPostProcess(
        const std::string& name,
        const std::string& mesh_name,
		const std::string& material_name)
    {
		PushBackSceneProcess(
			name,
			mesh_name,
			material_name,
			post_process_names_,
			post_process_mesh_ids_,
			post_process_material_ids_);
    }

	void Level::PushBackSceneProcess(
		const std::string& name,
		const std::string& mesh_name,
		const std::string& material_name,
		std::vector<std::string>& input_names,
		std::vector<EntityId>& input_mesh_ids,
		std::vector<EntityId>& input_material_ids)
	{
		auto maybe_mesh_id = GetIdFromName(mesh_name);
		if (!maybe_mesh_id)
		{
			throw std::runtime_error(
				fmt::format("{} has no id with name {}", name, mesh_name));
		}
		auto* mesh_node = GetSceneNodeFromId(maybe_mesh_id.value());
		if (!mesh_node)
		{
			throw std::runtime_error(
				fmt::format(
					"{} - {} is not a mesh with id {}", 
					name,
					mesh_name, 
					maybe_mesh_id.value()));
		}
		auto maybe_material_id = GetIdFromName(material_name);
		if (!maybe_material_id)
		{
			throw std::runtime_error(
				fmt::format("{} has no id with name {}", name, material_name));
		}
		auto* material = GetMaterialFromId(maybe_material_id.value());
		if (!material)
		{
			throw std::runtime_error(
				fmt::format(
					"{} - {} is not a materail with id {}",
					name,
					material_name,
					maybe_material_id.value()));
		}
		input_mesh_ids.push_back(maybe_mesh_id.value());
		input_material_ids.push_back(maybe_material_id.value());
		input_names.push_back(name);
		// Check all equal in size.
		const std::vector<size_t> sizes = 
		{ 
			input_mesh_ids.size(), 
			input_material_ids.size(), 
			input_names.size() 
		};
		const auto required_size = sizes.front();
        if (!std::all_of(std::begin(sizes), std::end(sizes),
            [required_size](const size_t& s) { return s == required_size; }))
		{
			throw std::runtime_error("size don't match!");
		}
	}

} // End namespace frame.
