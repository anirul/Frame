#include "Effect.h"
#include "Convert.h"

namespace sgl {

	Effect::Effect(
		const frame::proto::Effect& effect_proto,
		const std::map<std::string, std::shared_ptr<Texture>>& 
			name_texture_map)
	{
		name_ = effect_proto.name();
		for (const auto& name : effect_proto.input_textures_names())
			in_material_.AddTexture(name, name_texture_map.at(name));
		for (const auto& name : effect_proto.output_textures_names())
			out_material_.AddTexture(name, name_texture_map.at(name));
		shader_name_ = effect_proto.shader();
		uniforms_ = std::vector<frame::proto::Uniform>
		{
			effect_proto.parameters().begin(), 
			effect_proto.parameters().end() 
		};
	}

	void Effect::Startup(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const std::shared_ptr<UniformInterface> uniform_interface,
		const std::shared_ptr<Mesh> mesh)
	{
		uniform_interface_ = uniform_interface;
		size_ = size;
		render_.CreateStorage(size_);
		frame_.AttachRender(render_);
		program_ = CreateProgram(shader_name_);
		mesh_ = mesh;
		for (const auto& texture : out_material_.GetMap())
			frame_.AttachTexture(*texture.second);
		frame_.DrawBuffers(
			static_cast<std::uint32_t>(out_material_.GetMap().size()));
	}

	void Effect::Draw(const double dt /*= 0.0*/)
	{
		program_->Use();
		for (const frame::proto::Uniform& uniform : uniforms_)
			RegisterUniformFromProto(uniform, *uniform_interface_, *program_);
		ScopedBind scoped_frame(frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);
		mesh_->SetMaterial(in_material_);
		mesh_->Draw(program_);
	}

	const std::shared_ptr<sgl::ProgramInterface> Effect::GetProgram() const
	{
		return program_;
	}

} // End namespace sgl.
