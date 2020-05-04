#pragma once

#include <utility>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"

namespace sgl {

	class Render
	{
	public:
		Render();
		virtual ~Render();

	public:
		void Bind() const;
		void UnBind() const;
		void BindStorage(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelDepthComponent pixel_depth_component =
				PixelDepthComponent::DEPTH_COMPONENT24) const;

	public:
		const unsigned int GetId() const { return render_id_; }

	protected:
		unsigned int render_id_;
		const Error& error_ = Error::GetInstance();
	};

} // End namespace sgl.
