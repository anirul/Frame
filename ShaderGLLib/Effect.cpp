#include "Effect.h"
#include "Convert.h"

namespace sgl {

	Effect::Effect(
		const frame::proto::Effect& proto_effect,
		const std::map<std::string, std::shared_ptr<Texture>>& 
			name_texture_map)
	{
		name_ = proto_effect.name();
		for (const auto& name : proto_effect.input_textures_names())
			in_material_.AddTexture(name, name_texture_map.at(name));
		for (const auto& name : proto_effect.output_textures_names())
			out_material_.AddTexture(name, name_texture_map.at(name));
		program_ = CreateProgram(proto_effect.shader());
		uniforms_ = std::vector<frame::proto::Uniform>
		{
			proto_effect.parameters().begin(),
			proto_effect.parameters().end()
		};
		render_type_ = proto_effect.render_type();
	}

	void Effect::Startup(
		const std::pair<std::uint32_t, std::uint32_t> size,
		const std::shared_ptr<UniformInterface> uniform_interface)
	{
		uniform_interface_ = uniform_interface;
		size_ = size;
		render_.CreateStorage(size_);
		frame_.AttachRender(render_);
		for (const auto& texture : out_material_.GetMap())
			frame_.AttachTexture(*texture.second);
		frame_.DrawBuffers(
			static_cast<std::uint32_t>(out_material_.GetMap().size()));
	}

	void Effect::Draw(
		const std::shared_ptr<Mesh> mesh, 
		const double dt /*= 0.0*/)  const
	{
		program_->Use();
		for (const frame::proto::Uniform& uniform : uniforms_)
			RegisterUniformFromProto(uniform, *uniform_interface_, *program_);
		ScopedBind scoped_frame(frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		// Not sure about this.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);
		mesh->SetMaterial(in_material_);
		mesh->Draw(program_);
	}

} // End namespace sgl.
