# Repository Guidelines

## Project Structure & Module Organization
- Core engine sources live under `src/frame/`, with public headers mirrored in `include/frame/`. Keep module pairs synchronized to avoid API drift.
- Rendering backends are isolated per API (`src/frame/vulkan/`, `src/frame/opengl/`, etc.); add matching tests under `tests/frame/<feature>/` when extending a backend.
- Shared runtime assets belong in `asset/`; never commit generated artifacts or build outputs.
- Example apps compile from `examples/`; binaries should land in `build/<preset>/bin/`. Clean the `build/` tree before committing new work.

## Build, Test, and Development Commands
- `git submodule update --init --recursive` syncs dependencies in `external/`; run after clone, rebase, or submodule bumps.
- `cmake --preset windows` (MSVC) or `cmake --preset linux-debug` configures the project; add `--fresh` when toolchains change.
- `cmake --build --preset <preset> --target FrameVulkanTest` performs focused builds (omit `--target` for a full build).
- Run examples via `build/<preset>/bin/<Example>.exe -device <vulkan|opengl>`; Vulkan is the default renderer.

## Coding Style & Naming Conventions
- Follow `.clang-format` (Microsoft style, 4-space indent, 80-column soft limit) and `.editorconfig` (CRLF, no trailing whitespace).
- Classes use PascalCase, functions and locals camelCase, constants `kName`, filenames snake_case matching namespaces.
- Prefer ASCII source; introduce comments sparingly and only when the code is non-obvious.
- Keep headers in `include/frame/` aligned with their `src/` counterparts to preserve the public API.

## Testing Guidelines
- Primary GoogleTest suites: `FrameTest`, `FrameOpenGLTest`, `FrameVulkanTest`, plus JSON/file fixtures; co-locate new tests beside the feature under `tests/frame/`.
- Build tests with `cmake --build --preset <preset> --target <TestTarget>`; use `ctest --test-dir build/<preset> --output-on-failure -C Debug` before submission.
- Mirror Vulkan fixtures with OpenGL equivalents to maintain parity; seed data sits under `tests/frame/vulkan/`.
- Name tests after the scenario under test and the expected result for quick triage.

## Commit & Pull Request Guidelines
- Write imperative commit subjects under 72 characters.
- PR descriptions should state intent, list exercised presets/tests, link relevant issues, and include before/after imagery for graphics changes.
- Call out updates to `external/` or build scripts explicitly and confirm a clean configure/build cycle in the PR body.
- Exclude build outputs, compiled shaders, and other generated artifacts; update `.gitignore` when introducing new generators.

## Vulkan Port Progress

- JSON parsing is renderer-neutral: `frame::json::BuildLevelData` captures proto + asset root plus texture/program/material descriptors and a generated quad mesh stub.
- OpenGL builds its level via `frame::opengl::BuildLevel`, while Vulkan device stores the neutral data and calls `frame::vulkan::BuildLevel` (currently stubbed).
- Swapchain/resize plumbing works; Vulkan tests pass.
- Vulkan shaders (GLSL) live under `asset/shader/vulkan/`; `CMakeLists.txt` adds them for IDE visibility.
- Vulkan device compiles GLSL at runtime (shaderc). cmake now requires `shaderc` (via vcpkg).
- Next steps: implement GPU resource creation (vertex/index buffers, textures, descriptors, pipeline) in `Device::StartupFromLevelData` / `frame::vulkan::BuildLevel`, then draw the quad.
