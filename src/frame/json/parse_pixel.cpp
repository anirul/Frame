#include "frame/json/parse_pixel.h"

namespace frame::proto {

PixelElementSize PixelElementSize_BYTE() {
    PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(PixelElementSize::BYTE);
    return pixel_element_size;
}

PixelElementSize PixelElementSize_SHORT() {
    PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(PixelElementSize::SHORT);
    return pixel_element_size;
}

PixelElementSize PixelElementSize_HALF() {
    PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(PixelElementSize::HALF);
    return pixel_element_size;
}

PixelElementSize PixelElementSize_FLOAT() {
    PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(PixelElementSize::FLOAT);
    return pixel_element_size;
}

PixelStructure PixelStructure_GREY() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::GREY);
    return pixel_structure;
}

PixelStructure PixelStructure_GREY_ALPHA() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::GREY_ALPHA);
    return pixel_structure;
}

PixelStructure PixelStructure_RGB() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::RGB);
    return pixel_structure;
}

PixelStructure PixelStructure_RGB_ALPHA() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::RGB_ALPHA);
    return pixel_structure;
}

PixelStructure PixelStructure_BGR() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::BGR);
    return pixel_structure;
}

PixelStructure PixelStructure_BGR_ALPHA() {
    PixelStructure pixel_structure{};
    pixel_structure.set_value(PixelStructure::BGR_ALPHA);
    return pixel_structure;
}

bool operator==(const PixelStructure& l, const PixelStructure& r) { return l.value() == r.value(); }

bool operator==(const PixelElementSize& l, const PixelElementSize& r) {
    return l.value() == r.value();
}

}  // End namespace frame::proto.
