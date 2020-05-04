#include "ErrorTest.h"
#include "../ShaderGLLib/Window.h"

namespace test {

	TEST_F(ErrorTest, ThrowErrorTest)
	{
		auto window = sgl::CreateSDLOpenGL({ 320, 200 });
		// Disable the message box error reporting.
		error_.SetWindowPtr(nullptr);
		EXPECT_TRUE(window);
		EXPECT_EQ(GLEW_OK, glewInit());
		EXPECT_NO_THROW(error_.Display());
		unsigned int texture;
		glCreateTextures(GL_TEXTURE_2D, -1, &texture);
		EXPECT_THROW(error_.Display(), std::runtime_error);
	}

} // End namespace test.
