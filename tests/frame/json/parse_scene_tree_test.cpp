#include "frame/json/parse_scene_tree_test.h"

#include <stdexcept>

#include "frame/opengl/json/parse_scene_tree.h"
#include "frame/level.h"

namespace test
{

TEST_F(ParseSceneTreeTest, CreateParseSceneTreeTest)
{
    level_ = std::make_unique<frame::Level>();
    bool succeed = frame::json::ParseSceneTreeFile(
        proto_level_.scene_tree(),
        dynamic_cast<frame::LevelInterface&>(*level_.get()));
    EXPECT_TRUE(succeed);
    auto maybe_scene_id = level_->GetDefaultRootSceneNodeId();
    EXPECT_TRUE(maybe_scene_id);
    auto scene_id = maybe_scene_id;
    EXPECT_NE(0, scene_id);
    EXPECT_NO_THROW(auto& node = level_->GetSceneNodeFromId(scene_id));
}

TEST_F(ParseSceneTreeTest, RejectNodeMatrixWithoutExplicitMatrixType)
{
    auto proto_scene_tree = proto_level_.scene_tree();
    ASSERT_GT(proto_scene_tree.node_matrices_size(), 0);
    proto_scene_tree.mutable_node_matrices(0)->clear_matrix_type_enum();

    level_ = std::make_unique<frame::Level>();
    EXPECT_THROW(
        static_cast<void>(frame::json::ParseSceneTreeFile(
            proto_scene_tree,
            dynamic_cast<frame::LevelInterface&>(*level_.get()))),
        std::runtime_error);
}

TEST_F(ParseSceneTreeTest, RejectNodeLightWithoutExplicitLightType)
{
    auto proto_scene_tree = proto_level_.scene_tree();
    auto* proto_light = proto_scene_tree.add_node_lights();
    proto_light->set_name("missing-light-type");
    proto_light->mutable_position()->set_x(0.0f);
    proto_light->mutable_position()->set_y(0.0f);
    proto_light->mutable_position()->set_z(0.0f);
    proto_light->mutable_color()->set_x(1.0f);
    proto_light->mutable_color()->set_y(1.0f);
    proto_light->mutable_color()->set_z(1.0f);
    proto_light->clear_light_type();

    level_ = std::make_unique<frame::Level>();
    EXPECT_THROW(
        static_cast<void>(frame::json::ParseSceneTreeFile(
            proto_scene_tree,
            dynamic_cast<frame::LevelInterface&>(*level_.get()))),
        std::runtime_error);
}

} // End namespace test.
