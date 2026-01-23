#pragma once

#include "frame/json/proto.h"

namespace frame::json
{

/**
 * @brief BYTE 8bit long integer element.
 * @return A pixel element size byte.
 */
proto::PixelElementSize PixelElementSize_BYTE();
/**
 * @brief SHORT 16bit long integer element.
 * @return A pixel element size short.
 */
proto::PixelElementSize PixelElementSize_SHORT();
/**
 * @brief HALF 16bit long real element (IEEE-754).
 * @return A pixel element size half.
 */
proto::PixelElementSize PixelElementSize_HALF();
/**
 * @brief FLOAT 32bit long real element (IEEE-754).
 * @return A pixel element size float.
 */
proto::PixelElementSize PixelElementSize_FLOAT();
/**
 * @brief Comparison operator for pixel element size.
 * @param l: Left operand.
 * @param r: Right operand.
 * @return Are they equal or not?
 */
bool operator==(
    const proto::PixelElementSize& l, const proto::PixelElementSize& r);
/**
 * @brief Single value in a pixel also called R.
 * @return Pixel structure that will hold only one value.
 */
proto::PixelStructure PixelStructure_GREY();
/**
 * @brief Double value in a pixel also called RG
 * @return Pixel structure that will hold two values.
 */
proto::PixelStructure PixelStructure_GREY_ALPHA();
/**
 * @brief Triple value in a pixel also called RGB
 * @return Pixel structure that will hold tree values.
 */
proto::PixelStructure PixelStructure_RGB();
/**
 * @brief Quadruple value in a pixel also called RGBA
 * @return Pixel structure that will hold four values.
 */
proto::PixelStructure PixelStructure_RGB_ALPHA();
/**
 * @brief Triple value in a pixel also called RGB
 * @return Pixel structure that will hold tree values.
 */
proto::PixelStructure PixelStructure_BGR();
/**
 * @brief Quadruple value in a pixel also called RGBA
 * @return Pixel structure that will hold four values.
 */
proto::PixelStructure PixelStructure_BGR_ALPHA();
/**
 * @brief Comparison operator for pixel structure.
 * @param l: Left operand.
 * @param r: Right operand.
 * @return Are they equal or not?
 */
bool operator==(const proto::PixelStructure& l, const proto::PixelStructure& r);

} // End namespace frame::json.
