#include "ErrorTest.h"
#include "../ShaderGLLib/Window.h"

namespace test {

	TEST_F(ErrorTest, CreateErrorTest)
	{
		EXPECT_TRUE(error_);
		auto error2 = sgl::Error::GetInstance();
		EXPECT_EQ(error_, error2);
	}

	TEST_F(ErrorTest, ThrowErrorTest)
	{
		auto window = sgl::CreateSDLOpenGL({ 320, 200 });
		// Disable the message box error reporting.
		error_->SetWindowPtr(nullptr);
		EXPECT_TRUE(window);
		EXPECT_EQ(GLEW_OK, glewInit());
		unsigned int texture;
		glCreateTextures(GL_TEXTURE_2D, -1, &texture);
		EXPECT_THROW(error_->DisplayError(), std::runtime_error);
	}

} // End namespace test.
