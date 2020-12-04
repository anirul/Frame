#include "EffectTest.h"
#include <memory>
#include "UniformMock.h"
#include "../ShaderGLLib/Effect.h"

namespace test {

	using ::testing::StrictMock;

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

	TEST_F(EffectTest, EffectAndUniformTest)
	{
		frame::proto::Effect effect_proto;
		effect_proto.set_name("BlurTest");
		effect_proto.set_shader("Blur");
		frame::proto::Uniform uniform;
		uniform.set_name("test");
		uniform.set_uniform_float(1.0f);
		*effect_proto.add_parameters() = uniform;
		std::shared_ptr<sgl::Effect> effect_ptr = nullptr;
		std::map < std::string, std::shared_ptr<sgl::Texture>> empty_map{};
		EXPECT_NO_THROW(effect_ptr = 
			std::make_shared<sgl::Effect>(effect_proto, empty_map));
		EXPECT_TRUE(effect_ptr);
		auto uniform_mock_ptr = std::make_shared<StrictMock<UniformMock>>();
		std::pair<std::uint32_t, std::uint32_t> size = { 32, 32 };
		EXPECT_NO_THROW(effect_ptr->Startup(size, uniform_mock_ptr));
	}

	TEST_F(EffectTest, EffectFullTest)
	{
		frame::proto::Effect effect_proto;
		effect_proto.set_name("BlurTest");
		effect_proto.set_shader("Blur");
		frame::proto::Uniform uniform;
		uniform.set_name("exponent");
		uniform.set_uniform_float(2.2f);
		*effect_proto.add_parameters() = uniform;
		std::shared_ptr<sgl::Effect> effect_ptr = nullptr;
		std::map<std::string, std::shared_ptr<sgl::Texture>> in_out_map
		{
			{ 
				"Image", 
				std::make_shared<sgl::Texture>(
					std::make_pair<std::uint32_t, std::uint32_t>(32, 32)) 
			},
			{
				"frag_color",
				std::make_shared<sgl::Texture>(
					std::make_pair<std::uint32_t, std::uint32_t>(32, 32))
			},
		};
		*effect_proto.add_input_textures_names() = "Image";
		*effect_proto.add_output_textures_names() = "frag_color";
		EXPECT_NO_THROW(effect_ptr =
			std::make_shared<sgl::Effect>(effect_proto, in_out_map));
		EXPECT_TRUE(effect_ptr);
		auto uniform_mock_ptr = std::make_shared<StrictMock<UniformMock>>();
		std::pair<std::uint32_t, std::uint32_t> size = { 32, 32 };
		EXPECT_NO_THROW(effect_ptr->Startup(size, uniform_mock_ptr));
		EXPECT_NO_THROW(effect_ptr->Draw());
	}

} // End namespace test.
