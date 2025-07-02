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

TEST_F(WindowGlslFileTest, FailedApplyDoesNotTouchFile)
{
    frame::gui::WindowGlslFile window(
        temp_file_.string(), window_->GetDevice());
    window.SetEditorText("false");
    EXPECT_FALSE(window.Apply());

    std::ifstream in(temp_file_);
    std::string content(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, std::string("void main() {}"));
}

TEST_F(WindowGlslFileTest, ApplyWritesFileOnSuccess)
{
    frame::gui::WindowGlslFile window(
        temp_file_.string(), window_->GetDevice());
    const std::string shader =
        "#version 330 core\nlayout(location=0) out vec4 frag_color;\nvoid "
        "main(){frag_color=vec4(1.0);}";
    window.SetEditorText(shader);
    EXPECT_TRUE(window.Apply());

    std::ifstream in(temp_file_);
    std::string content(
        (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, shader);
}

} // namespace test
