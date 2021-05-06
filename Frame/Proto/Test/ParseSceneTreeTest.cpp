#include "Frame/Proto/Test/ParseSceneTreeTest.h"

#include "Frame/Proto/ParseSceneTree.h"
#include "Frame/Proto/ProtoLevelCreate.h"
#include "Frame/Level.h"

namespace test {

	TEST_F(ParseSceneTreeTest, CreateParseSceneTreeTest)
	{
		level_ = std::make_shared<frame::Level>();
		frame::proto::ParseSceneTreeFile(
			frame::proto::GetSceneFile(), 
			level_.get());
		EXPECT_FALSE(node_);
		auto scene_id = level_->GetDefaultRootSceneNodeId();
		EXPECT_NE(0, scene_id);
		EXPECT_NO_THROW(node_ = level_->GetSceneNodeMap().at(scene_id));
		EXPECT_TRUE(node_);
	}

} // End namespace test.
