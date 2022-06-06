#include <iostream>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

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
        Application app(frame::file::FindFile("Asset/Json/SceneSimple.json"),
                        frame::CreateSDLOpenGL({ 1280, 720 }));
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
