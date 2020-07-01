#pragma once

#include <utility>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"
#include "../ShaderGLLib/ScopedBind.h"

namespace sgl {

	class Render : public BindLock
	{
	public:
		Render();
		virtual ~Render();

	public:
		void Bind() const override;
		void UnBind() const override;
		// /!\ This will bind and unbind!
		void CreateStorage(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelDepthComponent pixel_depth_component =
				PixelDepthComponent::DEPTH_COMPONENT24) const;

	public:
		const unsigned int GetId() const { return render_id_; }

	protected:
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }

	private:
		unsigned int render_id_;
		const Error& error_ = Error::GetInstance();
		mutable bool locked_bind_ = false;
	};

} // End namespace sgl.
