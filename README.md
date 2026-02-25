# Frame

Frame is a C++23 3D engine with both Vulkan and OpenGL backends. Backend
selection is runtime-configurable, and the sample apps, editor, and tests are
built from the same engine code.

![A Scene rendering made with Frame.](examples/raytracing.png)

## Highlights

- Runtime backend switch: `--device=vulkan` or `--device=opengl`
- Automatic fallback from Vulkan to OpenGL when Vulkan startup fails
- Scene loading from JSON definitions in `asset/json/`
- Model loading (OBJ/PLY) and texture loading via stb-supported formats
- Sample apps under `examples/` and integration tests under `tests/frame/`

## Prerequisites

- Git (with submodule support)
- CMake 3.21+
- A C++23-capable compiler
  - Windows: Visual Studio 2022 (`v143`) recommended
  - Linux: GCC or Clang
- Ninja (for Linux presets)
- Vulkan loader/driver installed if running the Vulkan backend

## Setup

Clone and initialize external dependencies:

```sh
git submodule update --init --recursive
```

If your environment uses Git LFS assets, also run:

```sh
git lfs pull
```

## Build

### Windows

```sh
cmake --preset windows
cmake --build --preset windows-debug
# or
cmake --build --preset windows-release
```

### Linux

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
# or
cmake --preset linux-release
cmake --build --preset linux-release
```

## Run Examples

The samples are built into `build/<preset>/bin/`.

Windows example:

```sh
build/windows/bin/Debug/03_RayTracing.exe --device=vulkan
build/windows/bin/Debug/03_RayTracing.exe --device=opengl
```

Linux example:

```sh
./build/linux-debug/bin/03_RayTracing --device=vulkan
./build/linux-debug/bin/03_RayTracing --device=opengl
```

Available examples:

- `00_JapaneseFlag`
- `01_RayMarching`
- `02_Cubemap`
- `03_RayTracing`
- `04_RayTracingBvh`

Useful runtime flags:

- `--device={vulkan|opengl}`: choose rendering backend
- `--vk_validation={true|false}`: toggle Vulkan validation layers

## Run Tests

Linux:

```sh
cmake --build --preset linux-debug --target FrameTest FrameOpenGLTest FrameVulkanTest
ctest --test-dir build/linux-debug --output-on-failure
```

Windows:

```sh
cmake --build --preset windows-debug --target FrameTest FrameOpenGLTest FrameVulkanTest
ctest --test-dir build/windows -C Debug --output-on-failure
```

## Examples and Docs

- Example overview: [`examples/README.md`](examples/README.md)
- Engine source: `frame/`
- Editor application: `editor/`
