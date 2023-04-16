# Frame

3D engine example it use OpenGL and soon it will be using (Vulkan or DirectX
12). It take models in obj forms and images that can be read by stb (jpg, png,
hdr,...).

![A Scene rendering made with ShaderGL.](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png)

## Requirements

- *GIT* needed if you want to get the sources.
- *A compiler* needed for compilation.
  - Visual Studio Community edition should be enough.
  - You could also use clang or gcc...
- *cmake* you can get it [here](https://cmake.org/).
- *VCPKG* you can get it [here](https://github.com/Microsoft/vcpkg/)

## VCPKG

This project use *VCPKG* again so you should be able to build it! Basically
you have to install and setup *VCPKG*, you can get it
[here](https://github.com/Microsoft/vcpkg/). Use the `./boostrap.bat` or the
OS specific command to have it instlled on you OS.

Then you can install all the required packages, you can use the command line
tools provided later:

- abseil
- glm
- gtest
- happly
- imgui
- opengl
- protobuf
- sdl2
- spdlog
- stb
- tinyobjloader

## Building

To your install path using this commands.

```shell
[...]Frame> mkdir build
[...]Frame> cd build
[...]Frame/build> cmake .. -DCMAKE_TOOLCHAIN_FILE="[...]\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

Then you can either use the *Visual Studio* from your OS or use the command 
line.

```shell
Frame/build> cmake --build .
```

### makefiles

You can just use the `make` command.

## Examples

You can have a look at various examples [here](examples/README.md).
