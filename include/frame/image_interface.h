#pragma once

#include <glm/glm.hpp>

#include "frame/json/proto.h"

namespace frame {

	/**
	 * @class ImageInterface
	 * @brief this store the pointer and all the accessor to an image (file on
	 *        the HDD).
	 */
	struct ImageInterface {
		//! Virtual destructor.
		virtual ~ImageInterface() = default;
		/**
		 * @brief Get size of the image.
		 * @return The size of the image.
		 */
		virtual glm::uvec2 GetSize() const = 0;
		/**
		 * @brief Get the length of the image (should be size.x * size.y).
		 * @return The size of the image in linear.
		 */
		virtual int GetLength() const = 0;
		/**
		 * @brief Get a pointer to the underlying structure.
		 * @return A void pointer to the data structure.
		 */
		virtual const void* Data() const = 0;
		/**
		 * @brief Get a pointer to the underlying structure.
		 * @return A void pointer to the data structure.
		 */
		virtual void* Data() = 0;
		/**
		 * @brief Get pixel element size (BYTE/SHOT/HALF/FLOAT).
		 * @return The proto pixel element size.
		 */
		virtual proto::PixelElementSize GetPixelElementSize() const = 0;
		/**
		 * @brief Get pixel structure (R/RG/RGB/RGBA).
		 * @return The proto pixel structure.
		 */
		virtual proto::PixelStructure GetPixelStructure() const = 0;
		/**
		 * @brief Save an image to a file.
		 * @param file: The file to save the image to.
		 */
		virtual void SaveImageToFile(const std::string& file) const = 0;
		/**
		 * @brief Set the data pointer to the class, used to be able to save to
		 *        HDD.
		 * @param data: Pointer to the memory containing the RGB data of the
		 *        image.
		 * @warning Note that the class won't be the owner of the pointer so it
		 *          won't free it when destroyed!
		 */
		virtual void SetData(void* data) = 0;
	};

}  // End namespace frame.
