#include "frame/json/parse_pixel.h"

namespace frame::json
{

proto::PixelElementSize PixelElementSize_BYTE()
{
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(proto::PixelElementSize::BYTE);
    return pixel_element_size;
}

proto::PixelElementSize PixelElementSize_SHORT()
{
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(proto::PixelElementSize::SHORT);
    return pixel_element_size;
}

proto::PixelElementSize PixelElementSize_HALF()
{
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(proto::PixelElementSize::HALF);
    return pixel_element_size;
}

proto::PixelElementSize PixelElementSize_FLOAT()
{
    proto::PixelElementSize pixel_element_size{};
    pixel_element_size.set_value(proto::PixelElementSize::FLOAT);
    return pixel_element_size;
}

proto::PixelStructure PixelStructure_GREY()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::GREY);
    return pixel_structure;
}

proto::PixelStructure PixelStructure_GREY_ALPHA()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::GREY_ALPHA);
    return pixel_structure;
}

proto::PixelStructure PixelStructure_RGB()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::RGB);
    return pixel_structure;
}

proto::PixelStructure PixelStructure_RGB_ALPHA()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::RGB_ALPHA);
    return pixel_structure;
}

proto::PixelStructure PixelStructure_BGR()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::BGR);
    return pixel_structure;
}

proto::PixelStructure PixelStructure_BGR_ALPHA()
{
    proto::PixelStructure pixel_structure{};
    pixel_structure.set_value(proto::PixelStructure::BGR_ALPHA);
    return pixel_structure;
}

bool operator==(const proto::PixelStructure& l, const proto::PixelStructure& r)
{
    return l.value() == r.value();
}

bool operator==(
    const proto::PixelElementSize& l, const proto::PixelElementSize& r)
{
    return l.value() == r.value();
}

} // End namespace frame::json.
