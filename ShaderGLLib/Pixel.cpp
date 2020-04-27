#include "Pixel.h"
#include <string>
#include <stdexcept>
#include <GL/glew.h>

namespace sgl {

	int ConvertToGLType(const PixelElementSize pixel_element_size)
	{
		switch (pixel_element_size)
		{
		case PixelElementSize::BYTE:
			return GL_UNSIGNED_BYTE;
		case PixelElementSize::SHORT:
			return GL_UNSIGNED_SHORT;
		case PixelElementSize::HALF:
			return GL_HALF_FLOAT;
		case PixelElementSize::FLOAT:
			return GL_FLOAT;
		}
		throw 
			std::runtime_error(
				"unknown element size : " +	
				static_cast<int>(pixel_element_size));
	}

	int ConvertToGLType(const PixelStructure pixel_structure)
	{
		switch (pixel_structure)
		{
		case PixelStructure::GREY:
			return GL_RED;
		case PixelStructure::GREY_ALPHA:
			return GL_RG;
		case PixelStructure::RGB:
			return GL_RGB;
		case PixelStructure::RGB_ALPHA:
			return GL_RGBA;
		}
		throw 
			std::runtime_error(
				"unknown structure : " + static_cast<int>(pixel_structure));
	}

	int ConvertToGLType(
		const PixelElementSize pixel_element_size, 
		const PixelStructure pixel_structure)
	{
		switch (pixel_element_size)
		{
		case PixelElementSize::BYTE:
			switch (pixel_structure)
			{
			case PixelStructure::GREY:
				return GL_R8;
			case PixelStructure::GREY_ALPHA:
				return GL_RG8;
			case PixelStructure::RGB:
				return GL_RGB8;
			case PixelStructure::RGB_ALPHA:
				return GL_RGBA8;
			}
		case PixelElementSize::SHORT:
			switch (pixel_structure)
			{
			case PixelStructure::GREY:
				return GL_R16;
			case PixelStructure::GREY_ALPHA:
				return GL_RG16;
			case PixelStructure::RGB:
				return GL_RGB16;
			case PixelStructure::RGB_ALPHA:
				return GL_RGBA16;
			}
		case PixelElementSize::HALF:
			switch (pixel_structure)
			{
			case PixelStructure::GREY:
				return GL_R16F;
			case PixelStructure::GREY_ALPHA:
				return GL_RG16F;
			case PixelStructure::RGB:
				return GL_RGB16F;
			case PixelStructure::RGB_ALPHA:
				return GL_RGBA16F;
			}
		case PixelElementSize::FLOAT:
			switch (pixel_structure)
			{
			case PixelStructure::GREY:
				return GL_R32F;
			case PixelStructure::GREY_ALPHA:
				return GL_RG32F;
			case PixelStructure::RGB:
				return GL_RGB32F;
			case PixelStructure::RGB_ALPHA:
				return GL_RGBA32F;
			}
		}
		throw 
			std::runtime_error(
				"unknown structure : " + 
				std::to_string(static_cast<int>(pixel_structure)) + 
				" or element size : " + 
				std::to_string(static_cast<int>(pixel_element_size)));
	}

} // End namespace sgl.