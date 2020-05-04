#include "Mesh.h"
#include <iterator>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

namespace sgl {

	Mesh::Mesh(const std::string& file, const std::shared_ptr<Program>& program)
	{
		SetProgram(program);
		auto obj_file = LoadFromObj(file);
		std::vector<float> points = {};
		std::vector<float> normals = {};
		std::vector<float> texcoords = {};
		std::vector<std::int32_t> indices = {};
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

	sgl::Mesh::ObjFile Mesh::LoadFromObj(const std::string& file)
	{
		sgl::Mesh::ObjFile obj_file{};
		std::ifstream ifs;
		ifs.open(file, std::ifstream::in);
		if (!ifs.is_open())
		{
			throw std::runtime_error("Couldn't open file: " + file);
		}
		while (!ifs.eof()) 
		{
			std::string line = "";
			if (!std::getline(ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + file + " no token found.");
			}
			if (dump.size() > 2)
			{
				throw std::runtime_error(
					"Error parsing file: " + file + " token is too long.");
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
						glm::vec3 v(0, 0, 0);
						if (!(iss >> v.x))
						{
							throw std::runtime_error(
								"Error parsing file : " + 
								file + 
								" no x found in vn.");
						}
						if (!(iss >> v.y))
						{
							throw std::runtime_error(
								"Error parsing file : " +
								file +
								" no y found in vn.");
						}
						if (!(iss >> v.z))
						{
							throw std::runtime_error(
								"Error parsing file : " +
								file +
								" no z found in vn.");
						}
						obj_file.normals.push_back(v);
						break;
					}
					case 't':
					{
						assert(dump == "vt");
						glm::vec2 v(0, 0);
						if (!(iss >> v.x))
						{
							throw std::runtime_error(
								"Error parsing file : " +
								file +
								" no x found in vt.");
						}
						if (!(iss >> v.y))
						{
							throw std::runtime_error(
								"Error parsing file : " +
								file +
								" no y found in vt.");
						}
						obj_file.textures.push_back(v);
						break;
					}
					default:
					{
						throw std::runtime_error(
							"Error parsing file : " + 
							file +
							" unknown command : " +
							dump);
					}
					}
				}
				else
				{
					glm::vec3 v(0, 0, 0);
					if (!(iss >> v.x))
					{
						throw std::runtime_error(
							"Error parsing file : " +
							file +
							" no x found in v.");
					}
					if (!(iss >> v.y))
					{
						throw std::runtime_error(
							"Error parsing file : " +
							file +
							" no y found in v.");
					}
					if (!(iss >> v.z))
					{
						throw std::runtime_error(
							"Error parsing file : " +
							file +
							" no z found in v.");
					}
					obj_file.positions.push_back(v);
				}
			}
			break;
			case 'f':
			{
				if (dump.size() > 1)
				{
					throw std::runtime_error(
						"Error parsing file : " +
						file +
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
							file +
							" missing inner part of a description.");
					}
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
		if (obj_file.indices.size() % 3 != 0)
		{
			throw std::runtime_error(
				"Error parsing file : " +
				file +
				" indices are not triangles!");
		}
		return obj_file;
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
		return std::make_shared<Mesh>("../Asset/Model/Cube.obj", program);
	}

} // End namespace sgl.
