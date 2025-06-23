#include "frame/opengl/win32_opengl_none.h"

#include <GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <GL/wglew.h>
#include <windows.h>
#endif
#include <string>

#include "frame/opengl/message_callback.h"

namespace frame::opengl
{

Win32OpenGLNone::Win32OpenGLNone(glm::uvec2 size) : size_(size)
{
#if defined(_WIN32) || defined(_WIN64)
    hwnd_dummy_ = CreateWindowExW(
        0,
        L"STATIC",
        L"DummyWindow",
        WS_OVERLAPPED,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (!hwnd_dummy_)
        throw std::runtime_error("Couldn't create a dummy window pointer.");
    hdc_ = GetDC(hwnd_dummy_);
    if (!hdc_)
        throw std::runtime_error("Couldn't get an HDC.");
    GLuint pixel_format;
    static PIXELFORMATDESCRIPTOR pfd = {
        // clang-format off
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,  //< Flags
        PFD_TYPE_RGBA,  //< The kind of frame buffer. RGBA or palette.
        32,             //< Color depth of the frame buffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,  //< Number of bits for the depth buffer
        8,   //< Number of bits for the stencil buffer
        0,   //< Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
        // clang-format on
    };
    pixel_format = ChoosePixelFormat(hdc_, &pfd);
    if (!pixel_format)
        throw std::runtime_error("Couldn't initiate pixel format.");
    int ok = DescribePixelFormat(hdc_, pixel_format, sizeof(pfd), &pfd);
    if (!ok)
        throw std::runtime_error("Failed to describe OpenGL pixel format.");
    SetPixelFormat(hdc_, pixel_format, &pfd);
    HGLRC hglrc = wglCreateContext(hdc_);
    if (!hglrc)
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Couldn't get a context.");
    }
    ok = wglMakeCurrent(hdc_, hglrc);
    if (!ok)
        throw std::runtime_error(
            "Failed to make current OpenGL context from dummy window");
#else
    throw std::runtime_error("Not on windows.");
#endif
}

Win32OpenGLNone::~Win32OpenGLNone()
{
#if defined(_WIN32) || defined(_WIN64)
    HGLRC hglrc = wglGetCurrentContext();
    if (hglrc)
        wglDeleteContext(hglrc);
    wglMakeCurrent(hdc_, nullptr);
    ReleaseDC(hwnd_dummy_, hdc_);
#endif
}

void Win32OpenGLNone::Run(std::function<void()> lambda)
{
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Startup(size_);
    }
    if (input_interface_)
        input_interface_->NextFrame();
    device_->Display(0.0);
    for (const auto& plugin_interface : device_->GetPluginPtrs())
    {
        plugin_interface->Update(*device_.get(), 0.0);
    }
    lambda();
}

void* Win32OpenGLNone::GetGraphicContext() const
{
    // Initialize GLEW.
    GLenum error = GLEW_OK;
    if (GLEW_OK != (error = glewInit()))
    {
        throw std::runtime_error(
            reinterpret_cast<const char*>(glewGetErrorString(error)));
    }

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    // Check OpenGL versions.
    const char* version = (const char*)glGetString(GL_VERSION);
    logger_->info(std::format("Version  : {}", version));
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    logger_->info(std::format("Vendor   : {}", vendor));
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    logger_->info(std::format("Renderer : {}", renderer));

#if defined(_WIN32) || defined(_WIN64)
    if (wglewIsSupported("WGL_ARB_create_context"))
    {
        // GL_MAJOR_VERSION -> 4
        // GL_MINOR_VERSION -> 5
        // GL_CONTEXT_FLAGS -> GL_CONTEXT_FORWARD_COMPARIBLE_FLAG
        // GL_CONTEXT_PROFILE_MASK -> GL_CONTEXT_PROFILE_CORE
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB,
            4,
            WGL_CONTEXT_MINOR_VERSION_ARB,
            5,
            WGL_CONTEXT_FLAGS_ARB,
            GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            GL_CONTEXT_CORE_PROFILE_BIT,
            0};
        {
            HGLRC hglrc = wglGetCurrentContext();
            wglDeleteContext(hglrc);
        }
        HGLRC hglrc = wglCreateContextAttribsARB(hdc_, nullptr, attribs);
        wglMakeCurrent(hdc_, hglrc);
    }

    // Return current context.
    return wglGetCurrentContext();
#else
    throw std::runtime_error("Not on windows.");
#endif
}

} // End namespace frame::opengl.
