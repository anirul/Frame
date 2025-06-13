#include "frame/full/image_based_lighting_test.h"

#include <algorithm>

#include "frame/opengl/cubemap.h"

namespace test
{

TEST_F(ImageBasedLightingTest, SkyboxCubemapFromSingleFileTest)
{
    ASSERT_TRUE(level_);
    auto texture_id = level_->GetIdFromName("skybox");
    ASSERT_NE(frame::NullId, texture_id);
    auto& texture = level_->GetTextureFromId(texture_id);
    auto* cubemap = dynamic_cast<frame::opengl::Cubemap*>(&texture);
    ASSERT_NE(nullptr, cubemap);
    EXPECT_NE(0u, cubemap->GetId());
    EXPECT_GT(cubemap->GetSize().x, 0u);
    auto vec = cubemap->GetTextureFloat();
    EXPECT_EQ(cubemap->GetSize().x * cubemap->GetSize().y * 3 * 6, vec.size());
    auto range = std::minmax_element(vec.begin(), vec.end());
    EXPECT_NEAR(0.0f, *range.first, 0.1f);
    EXPECT_GT(*range.second, 0.0f) <<
        "Loaded cubemap contains only black pixels";
}

} // namespace test
