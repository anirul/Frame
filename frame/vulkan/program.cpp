#include "frame/vulkan/program.h"

#include <algorithm>
#include <exception>
#include <stdexcept>

#include "frame/uniform.h"

namespace frame::vulkan
{

Program::Program(const std::string& name)
{
    SetName(name);
}

void Program::AddInputTextureId(EntityId id)
{
    if (std::find(input_textures_.begin(), input_textures_.end(), id) !=
        input_textures_.end())
    {
        return;
    }
    input_textures_.push_back(id);
}

void Program::RemoveInputTextureId(EntityId id)
{
    input_textures_.erase(
        std::remove(input_textures_.begin(), input_textures_.end(), id),
        input_textures_.end());
}

std::vector<EntityId> Program::GetInputTextureIds() const
{
    return input_textures_;
}

void Program::AddOutputTextureId(EntityId id)
{
    if (std::find(output_textures_.begin(), output_textures_.end(), id) !=
        output_textures_.end())
    {
        return;
    }
    output_textures_.push_back(id);
}

void Program::RemoveOutputTextureId(EntityId id)
{
    output_textures_.erase(
        std::remove(output_textures_.begin(), output_textures_.end(), id),
        output_textures_.end());
}

std::vector<EntityId> Program::GetOutputTextureIds() const
{
    return output_textures_;
}

std::string Program::GetTemporarySceneRoot() const
{
    return temporary_scene_root_;
}

void Program::SetTemporarySceneRoot(const std::string& name)
{
    temporary_scene_root_ = name;
}

EntityId Program::GetSceneRoot() const
{
    return scene_root_;
}

void Program::SetSceneRoot(EntityId scene_root)
{
    scene_root_ = scene_root;
}

void Program::Use(
    const UniformCollectionInterface& uniform_collection_interface,
    const LevelInterface* /*level*/)
{
    is_used_ = true;
    const auto uniform_names = uniform_collection_interface.GetUniformNames();
    for (const auto& name : uniform_names)
    {
        const UniformInterface* uniform_ptr = nullptr;
        try
        {
            uniform_ptr = &uniform_collection_interface.GetUniform(name);
        }
        catch (const std::exception&)
        {
            continue;
        }
        const auto& uniform = *uniform_ptr;
        if (HasUniform(name))
        {
            uniforms_[name]->FromProto(uniform.ToProto());
        }
        else
        {
            auto duplicated = std::make_unique<frame::Uniform>(uniform);
            AddUniform(std::move(duplicated));
        }
    }
}

std::vector<std::string> Program::GetUniformNameList() const
{
    std::vector<std::string> names;
    names.reserve(uniforms_.size());
    for (const auto& [name, _] : uniforms_)
    {
        names.push_back(name);
    }
    return names;
}

const UniformInterface& Program::GetUniform(const std::string& name) const
{
    if (!HasUniform(name))
    {
        throw std::runtime_error("Uniform not found: " + name);
    }
    return *uniforms_.at(name);
}

void Program::AddUniform(std::unique_ptr<UniformInterface>&& uniform)
{
    if (!uniform)
    {
        return;
    }
    const std::string name = uniform->GetName();
    uniforms_[name] = std::move(uniform);
}

void Program::RemoveUniform(const std::string& name)
{
    uniforms_.erase(name);
}

bool Program::HasUniform(const std::string& name) const
{
    return uniforms_.count(name) != 0;
}

} // namespace frame::vulkan
