# Repository Guidelines

## Project Structure & Module Organization
- Core engine implementation lives in `src/frame/`; expose matching headers under `include/frame/`. Update both together to avoid API drift.
- Renderer backends are isolated under `src/frame/vulkan/`, `src/frame/opengl/`, etc. Add focused tests under `tests/frame/<feature>/` whenever a backend changes.
- Runtime assets belong in `asset/`; example apps assemble from `examples/` and output binaries into `build/<preset>/bin/`. Clean `build/` before committing.

## Build, Test, and Development Commands
- `git submodule update --init --recursive` ensures `external/` dependencies stay in sync after clone, rebase, or branch switches.
- Configure via `cmake --preset windows` (MSVC) or `cmake --preset linux-debug`; add `--fresh` after toolchain updates.
- `cmake --build --preset <preset>` builds the tree; append `--target FrameVulkanTest` (or similar) for focused targets.
- Run examples with `build/<preset>/bin/Sample.exe -device vulkan` (use `opengl` to switch).

## Coding Style & Naming Conventions
- Adopt `.clang-format` (Microsoft style, 4 spaces, 80-column soft limit) and `.editorconfig` (CRLF, trim trailing whitespace).
- Use PascalCase for classes, camelCase for functions/locals, `kName` for constants, and snake_case filenames that mirror namespaces.
- Prefer ASCII and add comments only when intent is non-obvious.

## Testing Guidelines
- GoogleTest suites live in `FrameTest`, `FrameOpenGLTest`, and `FrameVulkanTest`; place new suites beside their feature under `tests/frame/`.
- Build tests with `cmake --build --preset <preset> --target <TestTarget>` and run via `ctest --test-dir build/<preset> --output-on-failure -C Debug`.
- Mirror Vulkan fixtures with OpenGL equivalents; store seed data under `tests/frame/vulkan/`.

## Commit & Pull Request Guidelines
- Write imperative commit subjects under 72 characters and describe the change in the first line.
- PRs should explain intent, list exercised presets/tests, link issues, and include before/after imagery for graphics deltas.
- Call out edits to `external/` or build tooling explicitly and confirm a clean configure/build run.
- Exclude generated outputs (build artifacts, compiled shaders); extend `.gitignore` for any new generators.
