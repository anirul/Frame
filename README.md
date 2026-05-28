# Frame

Frame is a C++23 3D engine with both Vulkan and OpenGL backends. Backend
selection is runtime-configurable, and the sample apps and tests are built
from the same engine code.

![A Scene rendering made with Frame.](examples/raytracing.png)

## Highlights

- Runtime backend switch: `--device=vulkan` or `--device=opengl`
- Automatic fallback from Vulkan to OpenGL when Vulkan startup fails
- Scene loading from JSON definitions in `asset/json/`
- Model loading (glTF/glb) and texture loading via stb-supported formats
- Sample apps under `examples/` and integration tests under `tests/frame/`

## Prerequisites

- Git (with submodule support) and Git LFS for binary assets
- CMake 3.21+
- A C++23-capable compiler
  - Windows: Visual Studio 2022 (`v143`) recommended
  - Linux: GCC or Clang
- Ninja (for Linux presets)
- Vulkan loader/driver installed if running the Vulkan backend
- Linux Wayland development packages when building with the Linux presets
  (`libwayland-dev`, `libxkbcommon-dev`, `libegl1-mesa-dev`, and
  `libdecor-0-dev` on Debian-like distributions, or `wayland`,
  `wayland-protocols`, `libxkbcommon`, `mesa`, and `libdecor` with
  Homebrew/Linuxbrew). `libdecor` gives SDL client-side Wayland decorations, so
  sample windows keep their title text, FPS updates, and close button.

## Setup

Clone and initialize external dependencies:

```sh
git submodule update --init --recursive
```

Initialize Git LFS and fetch binary assets:

```sh
git lfs install
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

The presets keep vcpkg installs in `build/vcpkg_installed/` so fresh configure
directories can reuse the same dependency tree instead of reinstalling packages
per build folder.

### Linux

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
# or
cmake --preset linux-release
cmake --build --preset linux-release
```

The Linux presets use repository vcpkg overlays for a Wayland-first build. SDL3
is built without its default X11 feature, DBus is built without its default
systemd feature, and the Vulkan loader overlay disables XCB/Xlib WSI support.
On Homebrew/Linuxbrew systems, the triplet discovers the prefix from
`HOMEBREW_PREFIX` or `brew --prefix` so vcpkg can find global and formula-local
pkg-config metadata without committing local machine paths.

## Run Examples

The samples are built into `build/<preset>/bin/`.

Windows example:

```sh
build/windows/bin/Debug/01_Refraction.exe --device=vulkan
build/windows/bin/Debug/01_Refraction.exe --device=opengl
```

Linux example:

```sh
./build/linux-debug/bin/01_Refraction --device=vulkan
./build/linux-debug/bin/01_Refraction --device=opengl
```

Available examples:

- `00_Cubemap`
- `01_Refraction`
- `02_Dragon`
- `03_SkinnedMesh`

## Command-line Arguments

Samples accept the following runtime flags. The short `-flag=value` and Windows
`/flag=value` forms are normalized to the same options.

| Argument | Values | Default | Description |
| --- | --- | --- | --- |
| `--device` | `vulkan`, `opengl` | `vulkan` | Chooses the rendering backend. If Vulkan startup fails, the app falls back to OpenGL. |
| `--rendering` | `auto`, `raster`, `raytracing` | `auto` | Overrides the render path generated from the level JSON. `auto` keeps the level default. Aliases include `rasterise`, `rasterize`, and `raytrace`. |
| `--vk_validation` | `true`, `false` | `true` in debug, `false` in release | Enables Vulkan validation layers when using the Vulkan backend. |
| `--auto_exit_seconds` | number of seconds | `0.0` | Exits automatically after the given duration. `0.0` disables auto-exit. |
| `--screenshot_on_exit` | `true`, `false` | `false` | Saves `ScreenShot.png` before an auto-exit. |

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
