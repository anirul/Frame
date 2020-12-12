#pragma once

#include <glm/glm.hpp>
#include "../FrameProto/Proto.h"
#include "../ShaderGLLib/Camera.h"
#include "../ShaderGLLib/Error.h"
#include "../ShaderGLLib/Logger.h"
#include "../ShaderGLLib/Material.h"
#include "../ShaderGLLib/StaticMesh.h"
#include "../ShaderGLLib/Program.h"
#include "../ShaderGLLib/Texture.h"
#include "../ShaderGLLib/Uniform.h"

namespace sgl {

	class Effect {
	public:
		// You will be able to add stuff in the constructor of the derived
		// class. Some thing like mesh, program, mipmap, textures and also
		// functions.
		Effect(
			const frame::proto::Effect& effect_proto,
			const std::map<std::string, std::shared_ptr<Texture>>& 
				name_texture_map);

	public:
		// Startup the Effect this is where the effect is created (this will be
		// called only once at the beginning).
		void Startup(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const std::shared_ptr<UniformInterface> uniform_interface);
		// This is the draw interfaces.
		void Draw(
			const std::shared_ptr<StaticMesh> mesh, 
			const double dt = 0.0) const;

	public:
		// Get the name of the effect.
		const std::string& GetName() const { return name_; }
		const frame::proto::Effect::RenderEnum GetRenderInputType() const 
		{
			return render_input_type_;
		}
		const frame::proto::Effect::RenderEnum GetRenderOutputType() const
		{
			return render_output_type_;
		}

	private:
		frame::proto::Effect::RenderEnum render_input_type_ = 
			frame::proto::Effect::INVALID;
		frame::proto::Effect::RenderEnum render_output_type_ =
			frame::proto::Effect::INVALID;
		Error& error_ = Error::GetInstance();
		Logger& logger_ = Logger::GetInstance();
		std::string name_;
		std::string shader_name_;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		Material out_material_ = {};
		Material in_material_ = {};
		FrameBuffer frame_ = {};
		RenderBuffer render_ = {};
		std::shared_ptr<UniformInterface> uniform_interface_ = nullptr;
		std::shared_ptr<ProgramInterface> program_ = nullptr;
		std::vector<frame::proto::Uniform> uniforms_{};
	};

} // End namespace sgl.
