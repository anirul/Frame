#include "Effect.h"

namespace sgl {

	Effect::Effect(
		const frame::proto::Effect& effect_proto,
		std::map<std::string, std::shared_ptr<Texture>>& name_texture_map)
	{
		name_ = effect_proto.name();
		for (const auto& name : effect_proto.input_textures_names())
			in_material_.AddTexture(name, name_texture_map[name]);
		for (const auto& name : effect_proto.output_textures_names())
			out_material_.AddTexture(name, name_texture_map[name]);
		shader_name_ = effect_proto.shader();
		uniforms_ = std::vector<frame::proto::Uniform>{
			effect_proto.parameters().begin(), 
			effect_proto.parameters().end() 
		};
	}

	void Effect::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{
		size_ = size;
		frame_.AttachRender(render_);
		render_.CreateStorage(size_);
		program_ = Program::CreateProgram(shader_name_);
		program_->Use();
		for (const frame::proto::Uniform& uniform : uniforms_)
		{
			switch (uniform.value_case())
			{
				case frame::proto::Uniform::kInteger:
				{
					program_->UniformInt(uniform.name(), uniform.integer());
					break;
				}
				case frame::proto::Uniform::kBoolean:
				{
					program_->UniformBool(uniform.name(), uniform.boolean());
					break;
				}
				case frame::proto::Uniform::kReal:
				{
					program_->UniformFloat(uniform.name(), uniform.real());
					break;
				}
				case frame::proto::Uniform::kEnum:
				{
					// TODO(anirul): Parse the enum.
					break;
				}
				case frame::proto::Uniform::kVec2:
				case frame::proto::Uniform::kVec3:
				case frame::proto::Uniform::kVec4:
				case frame::proto::Uniform::kMat3:
				case frame::proto::Uniform::kMat4:
				case frame::proto::Uniform::kIntegers:
				case frame::proto::Uniform::kBools:
				case frame::proto::Uniform::kReals:
				case frame::proto::Uniform::kVec2S:
				case frame::proto::Uniform::kVec3S:
				case frame::proto::Uniform::kVec4S:
				case frame::proto::Uniform::kMat3S:
				case frame::proto::Uniform::kMat4S:
				default:

			}
		}
		quad_ = CreateQuadMesh(program_);
		for (const auto& texture : out_material_.GetMap())
			frame_.AttachTexture(*texture.second);
		frame_.DrawBuffers(out_material_.GetMap().size());
	}

	void Effect::Draw(const double dt /*= 0.0*/)
	{
		ScopedBind scoped_frame(frame_);
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);
		quad_->SetMaterial(in_material_);
		quad_->Draw();
	}

	void Effect::Delete() {}

	const std::string& Effect::GetName() const
	{
		return name_;
	}

} // End namespace sgl.
