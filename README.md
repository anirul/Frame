# Frame

3D engine example it use OpenGL and soon it will be using (Vulkan or DirectX
12). It take models in obj forms and images that can be read by stb (jpg, png,
hdr,...).

![SceneRendering](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png "A Scene rendering made with ShaderGL.")

## Requirements

- *GIT* needed if you want to get the sources.
- *A compiler* needed for compilation.
    - Visual Studio Community edition should be enough.
    - You could also use clang or gcc...
- *cmake* you can get it [here](https://cmake.org/).
- *VCPKG* you can get it [here](https://github.com/Microsoft/vcpkg/)

## VCPKG

This project use _VCPKG_ again so you should be able to build it! Basically 
you have to install and setup _VCPKG_, you can get it 
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

## Use

You have may programs that can be used:

### JapaneseFlag

![JapaneseFlag](https://github.com/anirul/Frame/raw/master/examples/japanese_flag.png "A rendering of the Japanese flag using shaders.")

This is a very bare bone Shader example that will open a window and display
the Japanese flag (using shaders).

### RayMarching

![RayMarching](https://github.com/anirul/Frame/raw/master/examples/ray_marching.png "A rendering of a sphere on a plane using raymaching shaders.")

This is a simple ray marching example, it will draw a sphere and compute the
shading and the shadow.

### Simple

![Simple](https://github.com/anirul/Frame/raw/master/examples/scene_simple.png "A rendering of an apple floating in the coulds.")

This will draw a cube map and an apple (simple just albedo).

### SceneRendering

This is a complete scene rendering. It loads `obj` from the `Asset/`
directory directly into the 3D software. This render the scene using
Physically Based Rendering, the parameters are fetch from `mtl` files.
It also add Screen Space Ambient Occlusion and Bloom effect before rendering.

![SceneRendering](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png "A Scene rendering made with ShaderGL.")
