#pragma once

#include <utility>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"
#include "../ShaderGLLib/Logger.h"
#include "../ShaderGLLib/ScopedBind.h"

namespace sgl {

	class Render : public BindLockInterface
	{
	public:
		Render();
		virtual ~Render();

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		// /!\ This will bind and unbind!
		void CreateStorage(
			const std::pair<std::uint32_t, std::uint32_t> size) const;

	public:
		const unsigned int GetId() const { return render_id_; }

	protected:
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }

	private:
		unsigned int render_id_ = 0;
		mutable bool locked_bind_ = false;
		const Error& error_ = Error::GetInstance();
		const Logger& logger_ = Logger::GetInstance();
	};

} // End namespace sgl.
