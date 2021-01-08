#pragma once

#include <utility>
#include "Frame/Error.h"
#include "Frame/Logger.h"
#include "Frame/OpenGL/Pixel.h"
#include "Frame/OpenGL/ScopedBind.h"

namespace frame::opengl {

	class RenderBuffer : public BindInterface
	{
	public:
		RenderBuffer();
		virtual ~RenderBuffer();

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		// /!\ This will bind and unbind!
		void CreateStorage(
			const std::pair<std::uint32_t, std::uint32_t> size) const;

	public:
		unsigned int GetId() const { return render_id_; }

	protected:
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }

	private:
		unsigned int render_id_ = 0;
		mutable bool locked_bind_ = false;
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace frame::opengl.
