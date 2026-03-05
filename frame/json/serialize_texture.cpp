#include "frame/json/serialize_texture.h"

#include "frame/file/file_system.h"
#include "frame/json/serialize_uniform.h"
#include "frame/opengl/cubemap.h"

namespace frame::json
{

proto::Texture SerializeTexture(TextureInterface& texture_interface)
{
    proto::Texture proto_texture = texture_interface.GetData();
    // Do not persist large runtime pixel blobs into level JSON files.
    // File-backed textures keep their file_name/file_names metadata.
    constexpr int kMaxInlineTextureBytes = 1024;
    if (proto_texture.has_pixels() &&
        proto_texture.pixels().size() > kMaxInlineTextureBytes)
    {
        proto_texture.clear_pixels();
    }
    return proto_texture;
}

} // End namespace frame::json.
