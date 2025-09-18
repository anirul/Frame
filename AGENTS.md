# Repository Guidelines

## Project Structure & Module Organization

Frame's runtime sources live in `src/frame`, grouped by subsystems such as `opengl`, `json`, `proto`, and node components. Public headers mirror this layout under `include/frame`. Engine assets and demo content sit in `asset/` and `examples/`. Tests mirror engine modules inside `tests/frame`, with extra fixtures under `tests/frame/{file,json,opengl}`. Third-party libraries are managed through `external/vcpkg`, so do not vendor additional copies under `src/`.

## Build, Test, and Development Commands
Run `git submodule update --init --recursive` after cloning to populate dependencies. Configure a build with `cmake --preset linux-debug` (or `linux-release`, `windows`) and let the preset pull the VCPKG toolchain. Compile everything via `cmake --build --preset linux-debug`. Execute the full suite with `ctest --output-on-failure --test-dir build/linux-debug` or launch an individual binary such as `build/linux-debug/tests/FrameTest`.

## Coding Style & Naming Conventions
Use C++17, four-space indentation, and brace-on-new-line for types and functions. Prefer descriptive PascalCase for classes (`Camera`, `WindowFactory`), camelCase for methods (`UpdateCameraVectors`) and free functions, and snake_case with trailing underscores for private data members. Keep includes ordered: local headers after standard/third-party. All new headers must live under `include/frame/...` and ship with an accompanying implementation. Run `clang-format` if available; mirror existing style found in `src/frame`.

## Testing Guidelines
Frame uses GoogleTest (see `tests/frame/CMakeLists.txt`). Add focused `*_test.cpp` files alongside the module under test and register them through the existing CMake targets. Write tests in the Arrange-Act-Assert style and keep fixture helpers in the `tests/frame` tree. Tests should run cleanly through `ctest` in both Debug and Release builds; add coverage for new branches that touch rendering or resource loading paths.

## Commit & Pull Request Guidelines
Follow the concise style visible in `git log`: imperative mood summaries (`Add preprocess program support...`), optionally prefixed with scopes such as `feat:` when meaningful. Reference tickets in the body and describe behavioral changes plus validation steps. Pull requests should include a short overview, rebuild/test evidence, and screenshots or clips when the change affects rendering output. Link related issues and call out any follow-up work so reviewers can triage quickly.
