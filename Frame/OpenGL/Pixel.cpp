#include "Pixel.h"
#include <string>
#include <stdexcept>
#include <GL/glew.h>

namespace frame::opengl {

	int ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size)
	{
		switch (pixel_element_size.value())
		{
		case frame::proto::PixelElementSize::BYTE:
			return GL_UNSIGNED_BYTE;
		case frame::proto::PixelElementSize::SHORT:
			return GL_UNSIGNED_SHORT;
		case frame::proto::PixelElementSize::HALF:
			return GL_FLOAT; // Not half float.
		case frame::proto::PixelElementSize::FLOAT:
			return GL_FLOAT;
		default:
			throw
				std::runtime_error(
					"unknown element size : " +
					std::to_string(
						static_cast<int>(pixel_element_size.value())));	
		}
	}

	int ConvertToGLType(
		const frame::proto::PixelStructure& pixel_structure)
	{
		switch (pixel_structure.value())
		{
		case frame::proto::PixelStructure::GREY:
			return GL_RED;
		case frame::proto::PixelStructure::GREY_ALPHA:
			return GL_RG;
		case frame::proto::PixelStructure::RGB:
			return GL_RGB;
		case frame::proto::PixelStructure::RGB_ALPHA:
			return GL_RGBA;
		default:
			throw
				std::runtime_error(
					"unknown structure : " +
					std::to_string(static_cast<int>(pixel_structure.value())));
		}
	}

	int ConvertToGLType(
		const frame::proto::PixelElementSize& pixel_element_size,
		const frame::proto::PixelStructure& pixel_structure)
	{
		switch (pixel_element_size.value())
		{
		case frame::proto::PixelElementSize::BYTE:
			switch (pixel_structure.value())
			{
			case frame::proto::PixelStructure::GREY:
				return GL_R8;
			case frame::proto::PixelStructure::GREY_ALPHA:
				return GL_RG8;
			case frame::proto::PixelStructure::RGB:
				return GL_RGB8;
			case frame::proto::PixelStructure::RGB_ALPHA:
				return GL_RGBA8;
			default:
				throw
					std::runtime_error(
						"unknown structure : " +
						std::to_string(
							static_cast<int>(pixel_structure.value())) +
						" or element size : " +
						std::to_string(
							static_cast<int>(pixel_element_size.value())));
			}
		case frame::proto::PixelElementSize::SHORT:
			switch (pixel_structure.value())
			{
			case frame::proto::PixelStructure::GREY:
				return GL_R16;
			case frame::proto::PixelStructure::GREY_ALPHA:
				return GL_RG16;
			case frame::proto::PixelStructure::RGB:
				return GL_RGB16;
			case frame::proto::PixelStructure::RGB_ALPHA:
				return GL_RGBA16;
			default:
				throw
					std::runtime_error(
						"unknown structure : " +
						std::to_string(
							static_cast<int>(pixel_structure.value())) +
						" or element size : " +
						std::to_string(
							static_cast<int>(pixel_element_size.value())));
			}
		case frame::proto::PixelElementSize::HALF:
			switch (pixel_structure.value())
			{
			case frame::proto::PixelStructure::GREY:
				return GL_R16F;
			case frame::proto::PixelStructure::GREY_ALPHA:
				return GL_RG16F;
			case frame::proto::PixelStructure::RGB:
				return GL_RGB16F;
			case frame::proto::PixelStructure::RGB_ALPHA:
				return GL_RGBA16F;
			default:
				throw
					std::runtime_error(
						"unknown structure : " +
						std::to_string(
							static_cast<int>(pixel_structure.value())) +
						" or element size : " +
						std::to_string(
							static_cast<int>(pixel_element_size.value())));
			}
		case frame::proto::PixelElementSize::FLOAT:
			switch (pixel_structure.value())
			{
			case frame::proto::PixelStructure::GREY:
				return GL_R32F;
			case frame::proto::PixelStructure::GREY_ALPHA:
				return GL_RG32F;
			case frame::proto::PixelStructure::RGB:
				return GL_RGB32F;
			case frame::proto::PixelStructure::RGB_ALPHA:
				return GL_RGBA32F;
			default:
				throw
					std::runtime_error(
						"unknown structure : " +
						std::to_string(
							static_cast<int>(pixel_structure.value())) +
						" or element size : " +
						std::to_string(
							static_cast<int>(pixel_element_size.value())));
			}
		default:
			throw
				std::runtime_error(
					"unknown structure : " +
					std::to_string(
						static_cast<int>(pixel_structure.value())) +
					" or element size : " +
					std::to_string(
						static_cast<int>(pixel_element_size.value())));
		}
	}

} // End namespace frame::opengl.
