#pragma once

#include <glm/glm.hpp>
#include <utility>

#include "frame/logger.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/scoped_bind.h"

namespace frame::opengl {

	/**
	 * @class RenderBuffer
	 * @brief Derived from bind interface, this is the class that is suppose to
	 * represent a render buffer.
	 */
	class RenderBuffer : public BindInterface {
	public:
		//! @brief Constructor create a render buffer.
		RenderBuffer();
		//! @brief Destructor destroy the render buffer.
		virtual ~RenderBuffer();

	public:
		/**
		 * @brief This will bind the render buffer to current context, this come
		 * from the bind interface.
		 * @param slot: Optional not use in this context.
		 */
		void Bind(const unsigned int slot = 0) const override;
		//! @brief Unbind the render buffer from the current context, this come
		//! from the bind interface.
		void UnBind() const override;
		/**
		 * @brief Create a storage in the render buffer, this will bind and
		 * unbind!
		 * @param size: Render buffer size.
		 */
		void CreateStorage(glm::uvec2 size) const;

	public:
		/**
		 * @brief Get id, come from the bind interface.
		 * @return Return the OpenGL id of the render interface.
		 */
		unsigned int GetId() const override { return render_id_; }

	protected:
		/**
		 * @brief Lock the bind for RAII interface to the bind interface.
		 */
		void LockedBind() const override { locked_bind_ = true; }
		/**
		 * @brief Unlock the bind for RAII interface to the bind interface.
		 */
		void UnlockedBind() const override { locked_bind_ = false; }

	private:
		unsigned int render_id_ = 0;
		mutable bool locked_bind_ = false;
		const Logger& logger_ = Logger::GetInstance();
	};

}  // End namespace frame::opengl.
