#include "frame/opengl/scene_simple_test.h"

#include <algorithm>

#include "frame/json/serialize_scene_tree.h"
#include "frame/uniform.h"
#include "frame/uniform_collection_wrapper.h"

namespace test
{

TEST_F(SceneSimpleTest, SerializeSceneTreeNodeNames)
{
    ASSERT_TRUE(level_);
    auto proto_scene_tree = frame::json::SerializeSceneTree(*level_);
    std::vector<std::string> expected_names;
    for (const auto& proto_mesh :
         proto_level_.scene_tree().node_static_meshes())
    {
        expected_names.push_back(proto_mesh.name());
    }
    std::vector<std::string> serialized_names;
    for (const auto& proto_mesh : proto_scene_tree.node_static_meshes())
    {
        serialized_names.push_back(proto_mesh.name());
    }
    EXPECT_EQ(expected_names.size(), serialized_names.size());
    for (const auto& name : expected_names)
    {
        EXPECT_NE(
            std::find(serialized_names.begin(), serialized_names.end(), name),
            serialized_names.end());
    }
}

TEST_F(SceneSimpleTest, ProgramUseKeepUniformEnums)
{
    ASSERT_TRUE(level_);
    auto program_ids = level_->GetPrograms();
    ASSERT_GT(program_ids.size(), 0);
    auto& program = level_->GetProgramFromId(program_ids[0]);
    auto names = program.GetUniformNameList();
    std::vector<frame::proto::Uniform::UniformEnum> enums;
    for (const auto& name : names)
    {
        enums.push_back(program.GetUniform(name).GetData().uniform_enum());
    }
    frame::UniformCollectionWrapper wrapper;
    for (const auto& name : names)
    {
        auto copy = std::make_unique<frame::Uniform>(program.GetUniform(name));
        wrapper.AddUniform(std::move(copy));
    }
    EXPECT_NO_THROW(program.Use(wrapper));
    EXPECT_EQ(names, program.GetUniformNameList());
    for (size_t i = 0; i < names.size(); ++i)
    {
        EXPECT_EQ(
            enums[i], program.GetUniform(names[i]).GetData().uniform_enum());
    }
}

} // namespace test
