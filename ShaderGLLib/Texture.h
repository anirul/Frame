#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <functional>
#include <map>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ShaderGLLib/Pixel.h"
#include "../ShaderGLLib/Error.h"
#include "../ShaderGLLib/Program.h"
#include "../ShaderGLLib/FrameBuffer.h"
#include "../ShaderGLLib/RenderBuffer.h"
#include "../ShaderGLLib/ScopedBind.h"
#include "../FrameProto/Proto.h"

namespace sgl {

	using TextureFilter = frame::proto::TextureFilter;
	TextureFilter TextureFilter_NEAREST();
	TextureFilter TextureFilter_LINEAR();
	TextureFilter TextureFilter_NEAREST_MIPMAP_NEAREST();
	TextureFilter TextureFilter_LINEAR_MIPMAP_NEAREST();
	TextureFilter TextureFilter_NEAREST_MIPMAP_LINEAR();
	TextureFilter TextureFilter_LINEAR_MIPMAP_LINEAR();
	TextureFilter TextureFilter_CLAMP_TO_EDGE();
	TextureFilter TextureFilter_MIRRORED_REPEAT();
	TextureFilter TextureFilter_REPEAT();

	int ConvertToGLType(const TextureFilter& texture_filter);
	TextureFilter ConvertFromGLType(int gl_filter);

	class Texture : public BindLockInterface
	{
	public:
		// Create an empty texture of size size.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create a texture from a file.
		Texture(
			const std::string& file,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a raw pointer.
		Texture(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const void* data,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a proto.
		Texture(
			const frame::proto::Texture& proto_texture, 
			const std::pair<std::uint32_t, std::uint32_t> size);
		virtual ~Texture();

	public:
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		virtual void BindEnableMipmap() const;
		virtual void SetMinFilter(const TextureFilter& texture_filter);
		virtual TextureFilter GetMinFilter() const;
		virtual void SetMagFilter(const TextureFilter& texture_filter);
		virtual TextureFilter GetMagFilter() const;
		virtual void SetWrapS(const TextureFilter& texture_filter);
		virtual TextureFilter GetWrapS() const;
		virtual void SetWrapT(const TextureFilter& texture_filter);
		virtual TextureFilter GetWrapT() const;
		void Clear(const glm::vec4 color);

	public:
		const int GetId() const { return texture_id_; }
		std::pair<std::uint32_t, std::uint32_t> GetSize() const 
		{ 
			return size_; 
		}
		const PixelElementSize GetPixelElementSize() const 
		{
			return pixel_element_size_; 
		}
		const PixelStructure GetPixelStructure() const
		{
			return pixel_structure_;
		}

	protected:
		void CreateTexture();
		Texture(
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB()) :
			pixel_element_size_(pixel_element_size),
			pixel_structure_(pixel_structure) {}
		void LockedBind() const override { locked_bind_ = true; }
		void UnlockedBind() const override { locked_bind_ = false; }
		friend class ScopedBind;

	protected:
		unsigned int texture_id_ = 0;
		std::pair<std::uint32_t, std::uint32_t> size_ = { 0, 0 };
		const PixelElementSize pixel_element_size_;
		const PixelStructure pixel_structure_;
		const Error& error_ = Error::GetInstance();
		mutable bool locked_bind_ = false;
		std::shared_ptr<RenderBuffer> render_ = nullptr;
		std::shared_ptr<FrameBuffer> frame_ = nullptr;
	};

	class TextureCubeMap : public Texture
	{
	public:
		// Create an empty cube map of the size size.
		TextureCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Take a single texture and map it to a cube view.
		TextureCubeMap(
			const std::string& file_name,
			const std::pair<std::uint32_t, std::uint32_t> size = { 512, 512 },
			const PixelElementSize& pixel_element_size =
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Take 6 texture to be mapped to the cube map, Order is:
		// right, left - (positive X, negative X)
		// top, bottom - (positive Y, negative Y)
		// front, back - (positive Z, negative Z)
		// The size is equal to the size of an image (*6).
		TextureCubeMap(
			const std::array<std::string, 6>& cube_file,
			const PixelElementSize& pixel_element_size = 
				PixelElementSize_BYTE(),
			const PixelStructure& pixel_structure = PixelStructure_RGB());
		// Create from a proto.
		// the size is the preferred size of the screen.
		TextureCubeMap(
			const frame::proto::Texture& texture,
			const std::pair<std::uint32_t, std::uint32_t> size);

	public:
		// Inside constructors to be called from proto and normal constructor.
		void InitCubeMap(
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size,
			const PixelStructure& pixel_structure);
		void InitCubeMapFromSixFiles(
			const std::array<std::string, 6> file_names,
			const PixelElementSize pixel_element_size,
			const PixelStructure pixel_structure);
		void InitCubeMapFromFile(
			const std::string& file_name,
			const std::pair<std::uint32_t, std::uint32_t> size,
			const PixelElementSize& pixel_element_size,
			const PixelStructure& pixel_structure);
		void Bind(const unsigned int slot = 0) const override;
		void UnBind() const override;
		void BindEnableMipmap() const override;
		void SetMinFilter(const TextureFilter& texture_filter) override;
		TextureFilter GetMinFilter() const override;
		void SetMagFilter(const TextureFilter& texture_filter) override;
		TextureFilter GetMagFilter() const override;
		void SetWrapS(const TextureFilter& texture_filter) override;
		TextureFilter GetWrapS() const override;
		void SetWrapT(const TextureFilter& texture_filter) override;
		TextureFilter GetWrapT() const override;
		void SetWrapR(const TextureFilter& texture_filter);
		TextureFilter GetWrapR() const;

	protected:
		// Create a cube map and assign it to the texture_id_.
		void CreateTextureCubeMap();
	};

} // End namespace sgl.
