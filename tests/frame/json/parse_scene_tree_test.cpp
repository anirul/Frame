#include "frame/json/parse_scene_tree_test.h"

#include "frame/level.h"
#include "frame/json/parse_scene_tree.h"

namespace test {

TEST_F(ParseSceneTreeTest, CreateParseSceneTreeTest) {
    level_       = std::make_unique<frame::Level>();
    bool succeed = frame::proto::ParseSceneTreeFile(
        proto_level_.scene_tree(), dynamic_cast<frame::LevelInterface*>(level_.get()));
    EXPECT_TRUE(succeed);
    auto maybe_scene_id = level_->GetDefaultRootSceneNodeId();
    EXPECT_TRUE(maybe_scene_id);
    auto scene_id = maybe_scene_id.value();
    EXPECT_NE(0, scene_id);
    frame::NodeInterface* node = nullptr;
    EXPECT_NO_THROW(node = level_->GetSceneNodeFromId(scene_id));
    EXPECT_TRUE(node);
}

}  // End namespace test.
