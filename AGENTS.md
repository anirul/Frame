# Repository Guidelines

## Project Structure & Module Organization
Engine sources and public headers now live side by side in `frame/`. Core interfaces (e.g., `frame/api.h`) sit at the root, while feature code stays grouped under `frame/opengl/`, `frame/vulkan/`, `frame/json/`, and similar modules. Update headers and implementations together inside these folders to keep exports consistent. Tests continue to mirror features under `tests/frame/<feature>/`, assets stay in `asset/`, example apps in `examples/`, and builds drop into `build/<preset>/bin/`. Clean `build/` before committing to avoid stray binaries.

## Build, Test, and Development Commands
Run `git submodule update --init --recursive` after branch switches to sync `external/` dependencies. Configure with `cmake --preset linux-debug` (add `--fresh` after toolchain updates) or `cmake --preset windows` on Windows hosts. Build via `cmake --build --preset <preset>`; append `--target FrameVulkanTest` or `FrameOpenGLTest` for focused suites. Launch samples from `build/<preset>/bin/Sample.exe -device vulkan` (swap `opengl` as needed) to validate runtime paths.

## Coding Style & Naming Conventions
Code follows `.clang-format` (Microsoft style, 4-space indent, 80-character soft limit) and `.editorconfig` (CRLF line endings, trimmed trailing whitespace). Adopt PascalCase for classes, camelCase for functions and locals, `kConstantName` for immutable values, and snake_case for filenames that mirror namespaces. Keep new sources ASCII unless feature requirements dictate otherwise, and add concise intent-driven comments only where behavior is non-obvious.

## Testing Guidelines
Tests use GoogleTest across `FrameTest`, `FrameOpenGLTest`, and `FrameVulkanTest`. Place fixtures beside related features under `tests/frame/`, mirroring Vulkan and OpenGL cases when applicable. Build and execute suites with `cmake --build --preset <preset> --target FrameVulkanTest` followed by `ctest --test-dir build/<preset> --output-on-failure -C Debug`. Store shared seeds under `tests/frame/vulkan/` to keep regression coverage aligned across backends.

## Commit & Pull Request Guidelines
Write imperative commit subjects under 72 characters and summarize intent on the first line. PR descriptions should capture motivation, enumerate exercised presets/tests, link tracked issues, and attach before/after imagery for any graphics-impacting change. Call out edits to `external/` or build tooling explicitly, confirm a clean configure/build run, and ensure generated binaries or intermediate artifacts stay out of version control (extend `.gitignore` when needed).
