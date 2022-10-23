#include <iostream>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "frame/common/application.h"
#include "frame/file/file_system.h"
#include "frame/file/image_stb.h"
#include "frame/window_factory.h"

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd) try {
#else
int main(int ac, char** av) try {
#endif
    frame::common::Application app(
        frame::CreateNewWindow(frame::WindowEnum::SDL2, frame::DeviceEnum::OPENGL, { 640, 480 }));
    app.Startup(frame::file::FindFile("asset/json/japanese_flag.json"));
    app.Run();
    return 0;
} catch (std::exception ex) {
#if defined(_WIN32) || defined(_WIN64)
    MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
    std::cerr << "Error: " << ex.what() << std::endl;
#endif
    return -2;
}
