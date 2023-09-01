#pragma once

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "frame/json/parse_pixel.h"
#include "frame/json/proto.h"
#include "frame/opengl/frame_buffer.h"
#include "frame/opengl/pixel.h"
#include "frame/opengl/program.h"
#include "frame/opengl/render_buffer.h"
#include "frame/opengl/scoped_bind.h"
#include "frame/texture_interface.h"

namespace frame::opengl {

	/**
	 * @brief Get texture frame from position.
	 * @param i: Position (+X, -X, +Y, -Y, +Z, -Z) [0, 5].
	 * @return The texture frame.
	 */
	proto::TextureFrame GetTextureFrameFromPosition(int i);

	/**
	 * @class TextureCubeMap
	 * @brief The same as texture but in a cube map perfect for environment
	 *        maps.
	 */
	class TextureCubeMap : public TextureInterface, public BindInterface {
	public:
		/**
		 * @brief Create a texture from a texture parameter structure.
		 * @param texture_parameter: Texture parameter structure.
		 */
		TextureCubeMap(const TextureParameter& texture_parameter);
		//! @brief Destroy texture also on GPU side.
		~TextureCubeMap();

	public:
		/**
		 * @brief Convert texture filter enum to GL type.
		 * @param texture_filter: Proto filter to be converted to OpenGL.
		 * @return OpenGL value.
		 */
		int ConvertToGLType(proto::TextureFilter::Enum texture_filter) const;
		/**
		 * @brief Convert texture OpenGL to filter enum.
		 * @param gl_filter: OpenGL value to be converted.
		 * @return Proto texture filter.
		 */
		proto::TextureFilter::Enum ConvertFromGLType(int gl_filter) const;
		/**
		 * @brief Get a copy of the texture output (8 bit format).
		 * @return A vector containing the pixel of the image in 8 bit format.
		 */
		std::vector<std::uint8_t> GetTextureByte() const override;
		/**
		 * @brief Get a copy of the texture output (16 bit format).
		 * @return A vector containing the pixel of the image in 16 bit format.
		 */
		std::vector<std::uint16_t> GetTextureWord() const override;
		/**
		 * @brief Get a copy of the texture output (32 bit format).
		 * @return A vector containing the pixel of the image in 32 bit format.
		 */
		std::vector<std::uint32_t> GetTextureDWord() const override;
		/**
		 * @brief Get a copy of the texture output (32 bit float format).
		 * @return A vector containing the pixel of the image in 32 bit float
		 *         format.
		 */
		std::vector<float> GetTextureFloat() const override;
		/**
		 * @brief Clear the texture (this is highly inefficient).
		 * @param color: Color to paint the texture to.
		 */
		void Clear(const glm::vec4 color) override;
		/**
		 * @brief Enable mip map, this allow a recursive level of texture faster
		 *        for rendering.
		 */
		void EnableMipmap() const override;
		/**
		 * @brief From the bind interface this will bind the texture at slot to
		 *        the current context.
		 * @param slot: Slot to be binded.
		 */
		void Bind(const unsigned int slot = 0) const override;
		/**
		 * @brief From the bind interface this will unbind the current texture
		 *        from the context.
		 */
		void UnBind() const override;
		/**
		 * @brief Set the minification filter.
		 * @param texture_filter: Usually and by default GL_LINEAR.
		 */
		void SetMinFilter(
			const proto::TextureFilter::Enum texture_filter) override;
		/**
		 * @brief Get the minification filter.
		 * @return The value of the minification filter.
		 */
		proto::TextureFilter::Enum GetMinFilter() const override;
		/**
		 * @brief Set the magnification filter.
		 * @param texture_filter: Usually and by default GL_LINEAR.
		 */
		void SetMagFilter(
			const proto::TextureFilter::Enum texture_filter) override;
		/**
		 * @brief Get the magnification filter.
		 * @return The value of the magnification filter.
		 */
		proto::TextureFilter::Enum GetMagFilter() const override;
		/**
		 * @brief Set the wrapping on the s size of the texture (horizontal)
		 *        this will decide how the texture is treated in case you
		 *        overflow in this direction.
		 * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE,
		 *        MIRRORED_REPEAT).
		 */
		void SetWrapS(const proto::TextureFilter::Enum texture_filter) override;
		/**
		 * @brief Get the wrapping on the s size of the texture (horizontal).
		 * @return The way the texture is wrap could be any of (REPEAT,
		 *         CLAMP_TO_EDGE, MIRRORED_REPEAT).
		 */
		proto::TextureFilter::Enum GetWrapS() const override;
		/**
		 * @brief Set the wrapping on the t size of the texture (vertical) this
		 *        will decide how the texture is treated in case you overflow in
		 *        this direction.
		 * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE,
		 *        MIRRORED_REPEAT).
		 */
		void SetWrapT(const proto::TextureFilter::Enum texture_filter) override;
		/**
		 * @brief Get the wrapping on the t size of the texture (vertical).
		 * @return The way the texture is wrap could be any of (REPEAT,
		 *         CLAMP_TO_EDGE, MIRRORED_REPEAT).
		 */
		proto::TextureFilter::Enum GetWrapT() const override;
		/**
		 * @brief Set the wrapping on the t size of the texture (vertical) this
		 *        will decide how the texture is treated in case you overflow in
		 *        this direction.
		 * @param texture_filter: Could be any of (REPEAT, CLAMP_TO_EDGE,
		 * MIRRORED_REPEAT).
		 */
		void SetWrapR(const proto::TextureFilter::Enum texture_filter);
		/**
		 * @brief Get the wrapping on the t size of the texture (vertical).
		 * @return The way the texture is wrap could be any of (REPEAT,
		 *         CLAMP_TO_EDGE, MIRRORED_REPEAT).
		 */
		proto::TextureFilter::Enum GetWrapR() const;
		/**
		 * @brief Copy the texture input to the texture.
		 * @param vector: Vector of uint32_t containing the RGBA values of the
		 * texture.
		 * @param size: Size of the image.
		 */
		void Update(std::vector<std::uint8_t>&& vector, glm::uvec2 size,
			std::uint8_t bytes_per_pixel) override;

	public:
		/**
		 * @brief Get name from the name interface.
		 * @return The name of the object.
		 */
		std::string GetName() const override { return name_; }
		/**
		 * @brief Set name from the name interface.
		 * @param name: New name to be set.
		 */
		void SetName(const std::string& name) override { name_ = name; }
		/**
		 * @brief Will respond true as this is a cube map.
		 * @return True this is a cube map.
		 */
		bool IsCubeMap() const final { return true; }
		/**
		 * @brief Return the texture cube map OpenGL id, from the bind
		 *        interface.
		 * @return Get the OpenGL id of the texture.
		 */
		unsigned int GetId() const override { return texture_id_; }
		/**
		 * @brief Get the texture cube map size.
		 * @return A single side size in pixel.
		 */
		glm::uvec2 GetSize() const override { return size_; }
		/**
		 * @brief Get the pixel element size individual element (BYTE, SHORT,
		 *        LONG, FLOAT).
		 * @return The pixel element size.
		 */
		proto::PixelElementSize::Enum GetPixelElementSize() const override {
			return pixel_element_size_.value();
		}
		/**
		 * @brief Get the pixel structure (R, RG, RGB, RGBA).
		 * @return the pixel structure.
		 */
		proto::PixelStructure::Enum GetPixelStructure() const override {
			return pixel_structure_.value();
		}

	protected:
		/**
		 * @brief Protected constructor this is a way to create a simple
		 *        structure from inside the class.
		 * @param pixel_element_size: Size of one of the element in a pixel
		 *        (BYTE, SHORT, HALF, FLOAT).
		 * @param pixel_element_structure: Structure of a pixel (R, RG, RGB,
		 *        RGBA).
		 */
		TextureCubeMap(
			const proto::PixelElementSize pixel_element_size =
			proto::PixelElementSize_BYTE(),
			const proto::PixelStructure pixel_structure =
				proto::PixelStructure_RGB())
			: pixel_element_size_(pixel_element_size),
			  pixel_structure_(pixel_structure) {}
		/**
		 * @brief Fill a texture with a data pointer, the size has to be set
		 *        first!
		 * @param data: pixel used to fill up (or null for don't care).
		 */
		void CreateTextureCubeMap(
			const std::array<void*, 6> cube_map =
				{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr });
		//! @brief Lock the bind for RAII interface to the bind interface.
		void LockedBind() const override { locked_bind_ = true; }
		//! @brief Unlock the bind for RAII interface to the bind interface.
		void UnlockedBind() const override { locked_bind_ = false; }

	protected:
		//! Create a render and a frame buffer for internal rendering (used in
		//! Clear).
		void CreateFrameAndRenderBuffer();
		friend class ScopedBind;

	private:
		unsigned int texture_id_ = 0;
		glm::uvec2 size_ = glm::uvec2(0, 0);
		const proto::PixelElementSize pixel_element_size_;
		const proto::PixelStructure pixel_structure_;
		mutable bool locked_bind_ = false;
		std::unique_ptr<RenderBuffer> render_ = nullptr;
		std::unique_ptr<FrameBuffer> frame_ = nullptr;
		std::string name_;
	};

}  // End namespace frame::opengl.
