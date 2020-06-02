#include "Device.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include "Frame.h"
#include "Render.h"

namespace sgl {

	Device::Device(
		void* gl_context, 
		const std::pair<std::uint32_t, std::uint32_t> size) : 
		gl_context_(gl_context),
		size_(size)
	{
		// Initialize GLEW.
		if (GLEW_OK != glewInit())
		{
			throw std::runtime_error("couldn't initialize GLEW");
		}

		// Enable blending to 1 - source alpha.
		glEnable(GL_BLEND);
		error_.Display(__FILE__, __LINE__ - 1);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		error_.Display(__FILE__, __LINE__ - 1);
		// Enable Z buffer.
		glEnable(GL_DEPTH_TEST);
		error_.Display(__FILE__, __LINE__ - 1);
		glDepthFunc(GL_LEQUAL);
		error_.Display(__FILE__, __LINE__ - 1);
		// Enable seamless cube map.
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		error_.Display(__FILE__, __LINE__ - 1);

		// Create programs.
		pbr_program_ = CreateProgram("PhysicallyBasedRendering");
		lighting_program_ = CreateProgram("Lighting");
	}

	void Device::Startup(const float fov /*= 65.0f*/)
	{
		fov_ = fov;
		SetupCamera();
	}

	void Device::Draw(const double dt)
	{
		Display(DrawTexture(dt));
	}

	void Device::Display(const std::shared_ptr<Texture>& texture)
	{
		auto program = CreateProgram("Display");
		auto quad = CreateQuadMesh(program);
		TextureManager texture_manager{};
		texture_manager.AddTexture("Display", texture);
		quad->SetTextures({ "Display" });
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad->Draw(texture_manager);
	}

	std::shared_ptr<Texture> Device::DrawTexture(const double dt)
	{
		auto texture = std::make_shared<Texture>(
			size_, 
			PixelElementSize::FLOAT, 
			PixelStructure::RGB);
		DrawMultiTextures({ texture }, dt);
		return texture;
	}

	void Device::DrawMultiTextures(
		const std::vector<std::shared_ptr<Texture>>& out_textures, 
		const double dt)
	{
		if (out_textures.empty())
		{
			throw std::runtime_error("Cannot draw on empty textures.");
		}

		// Setup the camera.
		SetupCamera();

		Frame frame{};
		Render render{};
		frame.BindAttach(render);
		render.BindStorage(size_);

		// Set the view port for rendering.
		glViewport(0, 0, size_.first, size_.second);
		error_.Display(__FILE__, __LINE__ - 1);

		// Clear the screen.
		glClearColor(.2f, 0.f, .2f, 1.0f);
		error_.Display(__FILE__, __LINE__ - 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		error_.Display(__FILE__, __LINE__ - 1);

		int i = 0;
		for (const auto& texture : out_textures)
		{
			frame.BindTexture(*texture, Frame::GetFrameColorAttachment(i));
			++i;
		}
		frame.DrawBuffers(static_cast<std::uint32_t>(out_textures.size()));

		for (const std::shared_ptr<Scene>& scene : scene_tree_)
		{
			const std::shared_ptr<Mesh>& mesh = scene->GetLocalMesh();
			if (!mesh)
			{
				continue;
			}

			// Draw the mesh.
			mesh->Draw(
				texture_manager_,
				perspective_,
				view_,
				scene->GetLocalModel(dt));
		}
	}

	void Device::SetupCamera()
	{
		// Set the perspective matrix.
		const float aspect =
			static_cast<float>(size_.first) / static_cast<float>(size_.second);
		perspective_ = glm::perspective(
			glm::radians(fov_),
			aspect,
			0.1f,
			100.0f);

		// Set the camera.
		view_ = camera_.GetLookAt();
	}

	void Device::LoadSceneFromObjFile(const std::string& obj_file)
	{
		if (obj_file.empty())
		{
			throw std::runtime_error(
				"Error invalid file name: " + obj_file);
		}
		std::string mtl_file = "";
		std::string mtl_path = obj_file;
		while (mtl_path.back() != '/' && mtl_path.back() != '\\')
		{
			mtl_path.pop_back();
		}
		std::ifstream obj_ifs(obj_file);
		if (!obj_ifs.is_open())
		{
			throw std::runtime_error(
				"Could not open file: " + obj_file);
		}
		std::string obj_content = "";
		while (!obj_ifs.eof())
		{
			std::string line = "";
			if (!std::getline(obj_ifs, line)) break;
			if (line.empty()) continue;
			std::istringstream iss(line);
			std::string dump;
			if (!(iss >> dump))
			{
				throw std::runtime_error(
					"Error parsing file: " + obj_file);
			}
			if (dump[0] == '#') continue;
			if (dump == "mtllib")
			{
				if (!(iss >> mtl_file))
				{
					throw std::runtime_error(
						"Error parsing file: " + obj_file);
				}
				mtl_file = mtl_path + mtl_file;
				continue;
			}
			obj_content += line + "\n";
		}
		std::ifstream mtl_ifs(mtl_file);
		if (!mtl_ifs.is_open())
		{
			throw std::runtime_error(
				"Error cannot open file: " + mtl_file);
		}
		scene_tree_ = LoadSceneFromObjStream(
			std::istringstream(obj_content),
			pbr_program_,
			obj_file);
		materials_ = LoadMaterialFromMtlStream(mtl_ifs, mtl_file);
	}

} // End namespace sgl.
