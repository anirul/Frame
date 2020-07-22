#include "Pixel.h"
#include <string>
#include <stdexcept>
#include <GL/glew.h>

namespace sgl {

	PixelElementSize PixelElementSize_BYTE()
	{
		PixelElementSize pixel_element_size{};
		pixel_element_size.set_value(PixelElementSize::BYTE);
		return pixel_element_size;
	}

	PixelElementSize PixelElementSize_SHORT()
	{
		PixelElementSize pixel_element_size{};
		pixel_element_size.set_value(PixelElementSize::SHORT);
		return pixel_element_size;
	}

	PixelElementSize PixelElementSize_HALF()
	{
		PixelElementSize pixel_element_size{};
		pixel_element_size.set_value(PixelElementSize::HALF);
		return pixel_element_size;
	}

	PixelElementSize PixelElementSize_FLOAT()
	{
		PixelElementSize pixel_element_size{};
		pixel_element_size.set_value(PixelElementSize::FLOAT);
		return pixel_element_size;
	}

	PixelStructure PixelStructure_GREY()
	{
		PixelStructure pixel_structure{};
		pixel_structure.set_value(PixelStructure::GREY);
		return pixel_structure;
	}

	PixelStructure PixelStructure_GREY_ALPHA()
	{
		PixelStructure pixel_structure{};
		pixel_structure.set_value(PixelStructure::GREY_ALPHA);
		return pixel_structure;
	}

	PixelStructure PixelStructure_RGB()
	{
		PixelStructure pixel_structure{};
		pixel_structure.set_value(PixelStructure::RGB);
		return pixel_structure;
	}

	PixelStructure PixelStructure_RGB_ALPHA()
	{
		PixelStructure pixel_structure{};
		pixel_structure.set_value(PixelStructure::RGB_ALPHA);
		return pixel_structure;
	}

	PixelDepthComponent PixelDepthComponent_DEPTH_COMPONENT8()
	{
		PixelDepthComponent pixel_depth_component{};
		pixel_depth_component.set_value(PixelDepthComponent::DEPTH_COMPONENT8);
		return pixel_depth_component;
	}

	PixelDepthComponent PixelDepthComponent_DEPTH_COMPONENT16()
	{
		PixelDepthComponent pixel_depth_component{};
		pixel_depth_component.set_value(PixelDepthComponent::DEPTH_COMPONENT16);
		return pixel_depth_component;
	}

	PixelDepthComponent PixelDepthComponent_DEPTH_COMPONENT24()
	{
		PixelDepthComponent pixel_depth_component{};
		pixel_depth_component.set_value(PixelDepthComponent::DEPTH_COMPONENT24);
		return pixel_depth_component;
	}

	PixelDepthComponent PixelDepthComponent_DEPTH_COMPONENT32()
	{
		PixelDepthComponent pixel_depth_component{};
		pixel_depth_component.set_value(PixelDepthComponent::DEPTH_COMPONENT32);
		return pixel_depth_component;
	}

	bool operator==(const PixelDepthComponent& l, const PixelDepthComponent& r)
	{
		return l.value() == r.value();
	}

	bool operator==(const PixelStructure& l, const PixelStructure& r)
	{
		return l.value() == r.value();
	}

	bool operator==(const PixelElementSize& l, const PixelElementSize& r)
	{
		return l.value() == r.value();
	}

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
		}
		throw
			std::runtime_error(
				"unknown element size : " +
				std::to_string(static_cast<int>(pixel_element_size.value())));
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
		}
		throw
			std::runtime_error(
				"unknown structure : " +
				std::to_string(static_cast<int>(pixel_structure.value())));
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
			}
		}
		throw
			std::runtime_error(
				"unknown structure : " +
				std::to_string(static_cast<int>(pixel_structure.value())) +
				" or element size : " +
				std::to_string(static_cast<int>(pixel_element_size.value())));
	}

	int ConvertToGLType(const PixelDepthComponent& pixel_depth_component)
	{
		switch (pixel_depth_component.value())
		{
			case PixelDepthComponent::DEPTH_COMPONENT8:
				return GL_DEPTH_COMPONENT;
			case PixelDepthComponent::DEPTH_COMPONENT16:
				return GL_DEPTH_COMPONENT16;
			case PixelDepthComponent::DEPTH_COMPONENT24:
				return GL_DEPTH_COMPONENT24;
			case PixelDepthComponent::DEPTH_COMPONENT32:
				return GL_DEPTH_COMPONENT32;
		}
		throw
			std::runtime_error(
				"unknown depth component : " + 
				std::to_string(
					static_cast<int>(pixel_depth_component.value())));
	}

} // End namespace sgl.
