#include "frame/json/serialize_texture.h"

#include "frame/file/file_system.h"
#include "frame/json/serialize_uniform.h"
#include "frame/opengl/cubemap.h"

namespace frame::json
{

proto::Texture SerializeTexture(TextureInterface& texture_interface)
{
    return texture_interface.GetData();
}

} // End namespace frame::json.
