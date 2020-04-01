#include "Mesh.h"
#include <iterator>
#include <fstream>
#include <sstream>
#include <gl/glew.h>
#include "Mesh.h"

namespace sgl {

	Mesh::Mesh(const std::string& file)
	{
		auto maybe_obj_file = LoadFromObj(file);
		if (!maybe_obj_file) 
		{
			throw std::runtime_error("Error could not read file: " + file);
		}
		auto obj_file = maybe_obj_file.value();
		std::vector<float> flat_positions_ = {};
		std::vector<float> flat_normals_ = {};
		std::vector<float> flat_textures_ = {};
		std::vector<unsigned int> flat_indices_ = {};
		std::vector<std::array<float, 8>> vertices;
		for (size_t i = 0; i < obj_file.indices.size(); ++i)
		{
			std::array<float, 8> v{};
			v[0] = obj_file.positions[obj_file.indices[i][0]].x;
			v[1] = obj_file.positions[obj_file.indices[i][0]].y;
			v[2] = obj_file.positions[obj_file.indices[i][0]].z;
			v[3] = obj_file.normals[obj_file.indices[i][2]].x;
			v[4] = obj_file.normals[obj_file.indices[i][2]].y;
			v[5] = obj_file.normals[obj_file.indices[i][2]].z;
			v[6] = obj_file.textures[obj_file.indices[i][1]].x;
			v[7] = obj_file.textures[obj_file.indices[i][1]].y;
			vertices.emplace_back(v);
			flat_indices_.push_back(static_cast<unsigned int>(i));
		}
		for (const std::array<float, 8>& v : vertices)
		{
			flat_positions_.push_back(v[0]);
			flat_positions_.push_back(v[1]);
			flat_positions_.push_back(v[2]);
			flat_normals_.push_back(v[3]);
			flat_normals_.push_back(v[4]);
			flat_normals_.push_back(v[5]);
			flat_textures_.push_back(v[6]);
			flat_textures_.push_back(v[7]);
		}

		// Position buffer initialization.
		point_buffer_.BindCopy(
			flat_positions_.size() * sizeof(float),
			flat_positions_.data());

		// Normal buffer initialization.
		normal_buffer_.BindCopy(
			flat_normals_.size() * sizeof(float),
			flat_normals_.data());
		
		// Texture coordinates buffer initialization.
		texture_buffer_.BindCopy(
			flat_textures_.size() * sizeof(float),
			flat_textures_.data());

		// Index buffer array.
		index_buffer_.BindCopy(
			flat_indices_.size() * sizeof(unsigned int),
			flat_indices_.data());

		// Get the size of the indices.
		index_size_ = static_cast<GLsizei>(flat_indices_.size());

		// Create a new vertex array (to render the mesh).
		glGenVertexArrays(1, &vertex_array_object_);
		glBindVertexArray(vertex_array_object_);
		point_buffer_.Bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		point_buffer_.UnBind();
		normal_buffer_.Bind();
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		normal_buffer_.UnBind();
		texture_buffer_.Bind();
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		texture_buffer_.UnBind();

		// Enable vertex attrib array.
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);
	}

	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &vertex_array_object_);
	}

	void Mesh::SetTextures(std::initializer_list<std::string> values)
	{
		textures_.clear();
		textures_.assign(values.begin(), values.end());
	}

	void Mesh::Draw(
		const sgl::Program& program,
		const sgl::TextureManager& texture_manager,
		const sgl::matrix& model /*= {}*/) const
	{
		texture_manager.DisableAll();
		for (const auto& str : textures_)
		{
			program.UniformInt(str, texture_manager.EnableTexture(str));
		}

		glBindVertexArray(vertex_array_object_);

		program.UniformMatrix("model", model);

		index_buffer_.Bind();
		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(index_size_),
			GL_UNSIGNED_INT,
			nullptr);
		index_buffer_.UnBind();

		glBindVertexArray(0);

		texture_manager.DisableAll();
	}

	std::optional<sgl::Mesh::ObjFile> Mesh::LoadFromObj(const std::string& file)
	{
		sgl::Mesh::ObjFile obj_file{};
		std::ifstream ifs;
		ifs.open(file, std::ifstream::in);
		if (!ifs.is_open())	return std::nullopt;
		while (!ifs.eof()) {
			std::string line = "";
			if (!std::getline(ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))	return std::nullopt;
			if (dump.size() > 2) return std::nullopt;
			switch (dump[0]) {
			case 'v':
			{
				if (dump.size() > 1)
				{
					switch (dump[1])
					{
					case 'n':
					{
						assert(dump == "vn");
						sgl::vector3 v(0, 0, 0);
						if (!(iss >> v.x)) return std::nullopt;
						if (!(iss >> v.y)) return std::nullopt;
						if (!(iss >> v.z)) return std::nullopt;
						obj_file.normals.push_back(v);
						break;
					}
					case 't':
					{
						assert(dump == "vt");
						sgl::vector2 v(0, 0);
						if (!(iss >> v.x)) return std::nullopt;
						if (!(iss >> v.y)) return std::nullopt;
						obj_file.textures.push_back(v);
						break;
					}
					default:
						return std::nullopt;
					}
				}
				else
				{
					if (dump != "v") return std::nullopt;
					sgl::vector3 v(0, 0, 0);
					if (!(iss >> v.x)) return std::nullopt;
					if (!(iss >> v.y)) return std::nullopt;
					if (!(iss >> v.z)) return std::nullopt;
					obj_file.positions.push_back(v);
				}
			}
			break;
			case 'f':
			{
				if (dump != "f") return std::nullopt;
				for (int i = 0; i < 3; ++i)
				{
					std::string inner;
					if (!(iss >> inner)) return std::nullopt;
					std::array<int, 3> vi;
					std::istringstream viss(inner);
					for (int& i : vi)
					{
						std::string inner_val;
						std::getline(viss, inner_val, '/');
						if (!inner_val.empty())
						{
							i = atoi(inner_val.c_str()) - 1;
						}
						else
						{
							i = -1;
						}
					}
					obj_file.indices.push_back(vi);
				}
			}
			break;
			default:
				break;
			}
		}
		if (obj_file.indices.size() % 3 != 0) return std::nullopt;
		return obj_file;
	}

} // End namespace sgl.
