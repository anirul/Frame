#include "frame/opengl/window_glsl_file_test.h"
#include <fstream>
#include <string>

namespace test
{

TEST_F(WindowGlslFileTest, FailedCompileDoesNotTouchFile)
{
    frame::gui::WindowGlslFile window(
        temp_file_.string(), window_->GetDevice());
    window.SetEditorText("false");
    EXPECT_FALSE(window.Compile());

    std::ifstream in(temp_file_);
    std::string content(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, std::string("void main() {}"));
}

} // namespace test
