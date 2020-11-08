#include "EffectTest.h"
#include <memory>
#include "../ShaderGLLib/Effect.h"

namespace test {

	TEST_F(EffectTest, ParseConstructorEffectTest)
	{
		frame::proto::Effect effect_proto;
		EXPECT_NO_THROW(sgl::Effect effect(effect_proto, {}));
	}

	TEST_F(EffectTest, CheckNameEffectTest)
	{
		frame::proto::Effect effect_proto;
		effect_proto.set_name("test");
		sgl::Effect effect(effect_proto, {});
		EXPECT_EQ("test", effect.GetName());
	}

} // End namespace test.
