# Frame

Welcome to Frame, a versatile 3D engine harnessing the power of OpenGL, with forthcoming support for Vulkan or DirectX 12. Designed to facilitate an immersive dive into computer graphics, Frame accepts models in OBJ format and images readable by stb (jpg, png, hdr, and more).

![A Scene rendering made with ShaderGL.](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png)

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

What you need to build and run Frame:

- *GIT* - Essential for cloning the repository. You can download it here.
- *Compiler* - Necessary for building the project. Options include:
  - Visual Studio (Community Edition is sufficient)
  - Alternatives like clang or gcc
- *CMake* - Required for creating the build system. Download it [here](https://cmake.org/).
- *VCPKG* - Frame uses VCPKG for managing C++ libraries. Set it up using instructions from the official [VCPKG GitHub](https://github.com/Microsoft/vcpkg/).

### Setting Up VCPKG

After installing VCPKG following the instructions [here](https://github.com/Microsoft/vcpkg/), use the command line to install the necessary packages:

```
./vcpkg install abseil glm gtest happly imgui opengl protobuf sdl2 spdlog stb tinyobjloader
```

Or you can use the cmake to get the list of dependencies from the `vcpkg.json`.

### Building Frame

Navigate to your Frame directory and use the following commands to build the project:

```shell
cd path-to-frame
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="path-to-vcpkg\scripts\buildsystems\vcpkg.cmake"
```

After setting up, you can build the project using Visual Studio or via the command line with the following command:

```shell
cmake --build .
```

## Dive in with Examples

Explore various practical examples to get accustomed to what Frame is capable of! Check them out [here](examples/README.md).
