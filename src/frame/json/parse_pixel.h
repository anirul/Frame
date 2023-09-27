#pragma once

#include "frame/json/proto.h"

namespace frame::proto
{

/**
 * @brief BYTE 8bit long integer element.
 * @return A pixel element size byte.
 */
PixelElementSize PixelElementSize_BYTE();
/**
 * @brief SHORT 16bit long integer element.
 * @return A pixel element size short.
 */
PixelElementSize PixelElementSize_SHORT();
/**
 * @brief HALF 16bit long real element (IEEE-754).
 * @return A pixel element size half.
 */
PixelElementSize PixelElementSize_HALF();
/**
 * @brief FLOAT 32bit long real element (IEEE-754).
 * @return A pixel element size float.
 */
PixelElementSize PixelElementSize_FLOAT();
/**
 * @brief Comparison operator for pixel element size.
 * @param l: Left operand.
 * @param r: Right operand.
 * @return Are they equal or not?
 */
bool operator==(const PixelElementSize &l, const PixelElementSize &r);
/**
 * @brief Single value in a pixel also called R.
 * @return Pixel structure that will hold only one value.
 */
PixelStructure PixelStructure_GREY();
/**
 * @brief Double value in a pixel also called RG
 * @return Pixel structure that will hold two values.
 */
PixelStructure PixelStructure_GREY_ALPHA();
/**
 * @brief Triple value in a pixel also called RGB
 * @return Pixel structure that will hold tree values.
 */
PixelStructure PixelStructure_RGB();
/**
 * @brief Quadruple value in a pixel also called RGBA
 * @return Pixel structure that will hold four values.
 */
PixelStructure PixelStructure_RGB_ALPHA();
/**
 * @brief Comparison operator for pixel structure.
 * @param l: Left operand.
 * @param r: Right operand.
 * @return Are they equal or not?
 */
bool operator==(const PixelStructure &l, const PixelStructure &r);

} // End namespace frame::proto.
