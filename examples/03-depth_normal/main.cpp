#include <iostream>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "Frame/File/FileSystem.h"
#include "Frame/File/Image.h"
#include "Frame/Window.h"
#include "Sample/Common/Application.h"

#if defined(_WIN32) || defined(_WIN64)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,
                   _In_ int nShowCmd)
#else
int main(int ac, char** av)
#endif
{
    try {
        // Would be nice to have an open file query.
        std::pair<std::uint32_t, std::uint32_t> size = { 0, 0 };
        {
            frame::file::Image image(frame::file::FindFile("./Asset/input.png"));
            size = image.GetSize();
        }
        Application app(frame::file::FindFile("Asset/Json/TextureModif.json"),
                        frame::CreateSDLOpenGL(size));
        app.Startup();
        app.Run();
    } catch (std::exception ex) {
#if defined(_WIN32) || defined(_WIN64)
        MessageBox(nullptr, ex.what(), "Exception", MB_ICONEXCLAMATION);
#else
        std::cerr << "Error: " << ex.what() << std::endl;
#endif
        return -2;
    }
    return 0;
}
