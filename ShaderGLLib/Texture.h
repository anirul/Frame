#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <map>
#include <array>
#include "../ShaderGLLib/Vector.h"

namespace sgl {

	class Texture 
	{
	public:
		Texture(const std::string& file);
		virtual ~Texture();
		void Bind(const unsigned int slot = 0) const;
		void UnBind() const;
		int GetId() const { return texture_id_; }
		std::pair<size_t, size_t> GetSize() const { return size_; }

	private:
		unsigned int texture_id_ = 0;
		std::pair<size_t, size_t> size_ = { 0, 0 };
	};

	class TextureManager 
	{
	public:
		TextureManager() = default;
		virtual ~TextureManager();
		bool AddTexture(
			const std::string& name, 
			const std::shared_ptr<sgl::Texture>& texture);
		bool RemoveTexture(const std::string& name);
		void EnableTexture(const std::string& name) const;
		void DisableTexture(const std::string& name) const;
		void DisableAll() const;

	private:
		std::map<std::string, std::shared_ptr<Texture>> name_texture_map_;
		mutable std::array<std::string, 32> name_array_;
	};

} // End namespace sgl.
