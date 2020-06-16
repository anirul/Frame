#include "Mesh.h"
#include <iterator>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

namespace sgl {

	Mesh::Mesh(
		std::istream& is,
		const std::string& name,
		const std::shared_ptr<Program>& program)
	{
		SetProgram(program);
		auto obj_file = LoadFromObj(is, name);
		material_name_ = obj_file.material;
		std::vector<float> points = {};
		std::vector<float> normals = {};
		std::vector<float> texcoords = {};
		std::vector<std::int32_t> indices = {};
		std::vector<std::array<float, 8>> vertices;
		for (size_t i = 0; i < obj_file.indices.size(); ++i)
		{
			const size_t min_position = obj_file.min_position;
			const size_t min_normal = obj_file.min_normal;
			const size_t min_texture = obj_file.min_texture;
			std::array<float, 8> v{};
			v[0] = obj_file.positions[obj_file.indices[i][0] - min_position].x;
			v[1] = obj_file.positions[obj_file.indices[i][0] - min_position].y;
			v[2] = obj_file.positions[obj_file.indices[i][0] - min_position].z;
			v[3] = obj_file.normals[obj_file.indices[i][2] - min_normal].x;
			v[4] = obj_file.normals[obj_file.indices[i][2] - min_normal].y;
			v[5] = obj_file.normals[obj_file.indices[i][2] - min_normal].z;
			v[6] = obj_file.textures[obj_file.indices[i][1] - min_texture].x;
			v[7] = obj_file.textures[obj_file.indices[i][1] - min_texture].y;
			vertices.emplace_back(v);
			indices.push_back(static_cast<unsigned int>(i));
		}
		for (const std::array<float, 8>& v : vertices)
		{
			points.push_back(v[0]);
			points.push_back(v[1]);
			points.push_back(v[2]);
			normals.push_back(v[3]);
			normals.push_back(v[4]);
			normals.push_back(v[5]);
			texcoords.push_back(v[6]);
			texcoords.push_back(v[7]);
		}
		CreateMeshFromFlat(points, normals, texcoords, indices);
	}

	
	Mesh::Mesh(
		const std::vector<float>& points,
		const std::vector<float>& normals,
		const std::vector<float>& texcoords,
		const std::vector<std::int32_t>& indices,
		const std::shared_ptr<Program>& program)
	{
		SetProgram(program);
		CreateMeshFromFlat(points, normals, texcoords, indices);
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

	void Mesh::SetTextures(const std::vector<std::string>& vec)
	{
		textures_.clear();
		textures_.assign(vec.begin(), vec.end());
	}

	void Mesh::SetProgram(const std::shared_ptr<Program>& program)
	{
		if (!program)
		{
			throw std::runtime_error("no program set.");
		}
		program_ = program;
	}

	void Mesh::CreateMeshFromFlat(
		const std::vector<float>& points,
		const std::vector<float>& normals,
		const std::vector<float>& texcoords,
		const std::vector<std::int32_t>& indices)
	{
		// Position buffer initialization.
		point_buffer_.BindCopy(
			points.size() * sizeof(float),
			points.data());

		// Normal buffer initialization.
		normal_buffer_.BindCopy(
			normals.size() * sizeof(float),
			normals.data());

		// Texture coordinates buffer initialization.
		texture_buffer_.BindCopy(
			texcoords.size() * sizeof(float),
			texcoords.data());

		// Index buffer array.
		index_buffer_.BindCopy(
			indices.size() * sizeof(std::int32_t),
			indices.data());

		// Get the size of the indices.
		index_size_ = static_cast<GLsizei>(indices.size());

		// Create a new vertex array (to render the mesh).
		glGenVertexArrays(1, &vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindVertexArray(vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);
		point_buffer_.Bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		point_buffer_.UnBind();
		normal_buffer_.Bind();
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		normal_buffer_.UnBind();
		texture_buffer_.Bind();
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		error_.Display(__FILE__, __LINE__ - 1);
		texture_buffer_.UnBind();

		// Enable vertex attrib array.
		glEnableVertexAttribArray(0);
		error_.Display(__FILE__, __LINE__ - 1);
		glEnableVertexAttribArray(1);
		error_.Display(__FILE__, __LINE__ - 1);
		glEnableVertexAttribArray(2);
		error_.Display(__FILE__, __LINE__ - 1);
		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);
	}

	void Mesh::Draw(
		const sgl::TextureManager& texture_manager,
		const glm::mat4& projection /*= glm::mat4(1.0f)*/,
		const glm::mat4& view /*= glm::mat4(1.0f)*/,
		const glm::mat4& model /*= glm::mat4(1.0f)*/) const
	{
		texture_manager.DisableAll();
		if (!program_)
		{
			throw std::runtime_error("program is not set.");
		}
		program_->Use();
		for (const auto& str : textures_)
		{
			program_->UniformInt(str, texture_manager.EnableTexture(str));
		}

		glBindVertexArray(vertex_array_object_);
		error_.Display(__FILE__, __LINE__ - 1);

		// Push updated matrices.
		program_->UniformMatrix("projection", projection);
		program_->UniformMatrix("view", view);
		program_->UniformMatrix("model", model);

		index_buffer_.Bind();
		glDrawElements(
			GL_TRIANGLES,
			static_cast<GLsizei>(index_size_),
			GL_UNSIGNED_INT,
			nullptr);
		error_.Display(__FILE__, __LINE__ - 5);
		index_buffer_.UnBind();

		glBindVertexArray(0);
		error_.Display(__FILE__, __LINE__ - 1);

		texture_manager.DisableAll();

		if (clear_depth_buffer_)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			error_.Display(__FILE__, __LINE__ - 1);
		}
	}

	Mesh::ObjFile Mesh::LoadFromObj(
		std::istream& is, 
		const std::string& name)
	{
		Mesh::ObjFile obj_file{};
		while (!is.eof()) 
		{
			std::string line = "";
			if (!std::getline(is, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + name + " no token found.");
			}
			if (dump.size() > 2)
			{
				if (dump == "usemtl")
				{
					std::string material = "";
					iss >> material;
					if (material.empty())
					{
						throw std::runtime_error(
							"Error parsing file: " + 
							name + 
							" cannot get material name.");
					}
					if (!obj_file.material.empty())
					{
						throw std::runtime_error(
							"Error parsing file: " +
							name +
							" material was already defined as: " +
							obj_file.material +
							" is redefined as : " +
							material);
					}
					obj_file.material = material;
					continue;
				}
				throw std::runtime_error(
					"Error parsing file: " + name + " token is too long.");
			}
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
						obj_file.normals.push_back(
							GetVec3From3Float(iss, name, dump));
						break;
					}
					case 't':
					{
						assert(dump == "vt");
						obj_file.textures.push_back(
							Getvec2From2Float(iss, name, dump));
						break;
					}
					default:
					{
						throw std::runtime_error(
							"Error parsing file : " + 
							name +
							" unknown command : " +
							dump);
					}
					}
				}
				else
				{
					obj_file.positions.push_back(
						GetVec3From3Float(iss, name, dump));
				}
			}
			break;
			case 'f':
			{
				if (dump.size() > 1)
				{
					throw std::runtime_error(
						"Error parsing file : " +
						name +
						" unknown command : " +
						dump);
				}
				for (int i = 0; i < 3; ++i)
				{
					std::string inner;
					if (!(iss >> inner))
					{
						throw std::runtime_error(
							"Error parsing file : " +
							name +
							" missing inner part of a description.");
					}
					std::array<int, 3> vi;
					std::istringstream viss(inner);
					auto lambda_set_minimum = [&obj_file](int index, int value)
					{
						switch (index)
						{
							case 0:
								obj_file.min_position = 
									std::min(value, obj_file.min_position);
								return;
							case 1:
								obj_file.min_texture =
									std::min(value, obj_file.min_texture);
								return;
							case 2:
								obj_file.min_normal = 
									std::min(value, obj_file.min_normal);
								return;
						}
						throw std::runtime_error(
							"Invalid index should be (0 - 2) is: " + index);
					};
					for (int i = 0; i < 3; ++i)
					{
						std::string inner_val;
						std::getline(viss, inner_val, '/');
						if (!inner_val.empty())
						{
							int val = atoi(inner_val.c_str()) - 1;
							lambda_set_minimum(i, val);
							vi[i] = val;
						}
						else
						{
							vi[i] = -1;
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
		if (obj_file.indices.size() % 3 != 0)
		{
			throw std::runtime_error(
				"Error parsing file : " +
				name +
				" indices are not triangles!");
		}
		return obj_file;
	}

	glm::vec3 Mesh::GetVec3From3Float(
		std::istream& is, 
		const std::string& stream_name, 
		const std::string& element_name) const
	{
		glm::vec3 v3(0, 0, 0);
		if (!(is >> v3.x))
		{
			throw std::runtime_error(
				"Error parsing file : " +
				stream_name +
				" no x found in " +
				element_name);
		}
		if (!(is >> v3.y))
		{
			throw std::runtime_error(
				"Error parsing file : " +
				stream_name +
				" no y found in " +
				element_name);
		}
		if (!(is >> v3.z))
		{
			throw std::runtime_error(
				"Error parsing file : " +
				stream_name +
				" no z found in " +
				element_name);
		}
		return v3;
	}

	glm::vec2 Mesh::Getvec2From2Float(
		std::istream& is, 
		const std::string& stream_name, 
		const std::string& element_name) const
	{
		glm::vec2 v2(0, 0);
		if (!(is >> v2.x))
		{
			throw std::runtime_error(
				"Error parsing file : " +
				stream_name +
				" no x found in " +
				element_name);
		}
		if (!(is >> v2.y))
		{
			throw std::runtime_error(
				"Error parsing file : " +
				stream_name +
				" no y found in " +
				element_name);
		}
		return v2;
	}

	std::shared_ptr<sgl::Mesh> CreateQuadMesh(
		const std::shared_ptr<Program>& program)
	{
		std::vector<float> points =
		{
			-1.f, 1.f, 0.f,
			1.f, 1.f, 0.f,
			-1.f, -1.f, 0.f,
			1.f, -1.f, 0.f,
		};
		std::vector<float> normals =
		{
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
		};
		std::vector<float> texcoords =
		{
			0, 1,
			1, 1,
			0, 0,
			1, 0,
		};
		std::vector<std::int32_t> indices =
		{
			0, 1, 2,
			1, 3, 2,
		};
		return std::make_shared<Mesh>(
			points, 
			normals, 
			texcoords, 
			indices, 
			program);
	}

	std::shared_ptr<sgl::Mesh> CreateCubeMesh(
		const std::shared_ptr<Program>& program)
	{
		return CreateMeshFromObjFile("../Asset/Model/Cube.obj", program);
	}

	std::shared_ptr<sgl::Mesh> CreateMeshFromObjFile(
		const std::string& file_path, 
		const std::shared_ptr<Program>& program)
	{
		auto ifs = std::ifstream(file_path);
		if (!ifs.is_open())
		{
			throw std::runtime_error("could not open file: " + file_path);
		}
		return std::make_shared<sgl::Mesh>(ifs, file_path, program);
	}

} // End namespace sgl.
