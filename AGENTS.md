# Repository Guidelines

## Project Structure & Module Organization
Frame's engine sources live in `src/frame`, with public headers mirrored under `include/frame`. Runtime assets such as models, textures, and shaders belong in `asset/`, while generated artifacts should never be committed. Runnable integration samples reside in `examples/`, and unit tests live in `tests/frame` with feature-specific subfolders (for example `tests/frame/json/`). CMake support scripts are under `cmake/`, and the VCPKG submodule is pulled into `external/` during configuration. Build products stay within `build/<preset>`; keep that tree clean in Git.

## Build, Test, and Development Commands
Initialize dependencies with `git submodule update --init --recursive`. Configure a build via `cmake --preset linux-debug` (use `linux-release` or `windows` as needed). Compile with `cmake --build --preset linux-debug`, and rebuild specific targets such as `FrameTest` using `cmake --build --preset linux-debug --target FrameTest`. Launch the sample viewer at `./build/linux-debug/examples/frame_viewer` after a successful build.

## Coding Style & Naming Conventions
C++ code follows the repository `.clang-format` (Microsoft base, 4-space indent, 80-column limit, left-aligned pointers). Public classes use PascalCase, while functions, variables, and private members stay camelCase; constants take the `kPrefix`. Filenames use snake_case and mirror namespace layout. Run `clang-format -i path/to/file.cpp` before committing and respect `.editorconfig` CRLF endings and trimmed whitespace.

## Testing Guidelines
GoogleTest drives unit coverage in `tests/frame`. Name suites after the component (for example `CameraTest`) and individual tests in `Method_State_Expectation` form. After building, execute `ctest --test-dir build/linux-debug --output-on-failure` to run the full suite; target the matching preset directory for other builds. Add targeted tests or update examples whenever you add features.

## Commit & Pull Request Guidelines
Write commit messages in imperative mood under 72 characters (e.g., `Update material parser defaults`). Group related changes and avoid WIP commits. Pull requests should describe intent, state which build or test presets were run, and link any relevant issues. Include before/after renders when modifying assets or graphics-facing code, and flag reviewers responsible for the subsystem touched.

## Assets & Dependencies
Coordinate before adding binaries over ~10 MB and keep large artifacts in `asset/` or approved storage. When adjusting VCPKG ports or submodules, note the change in your PR and confirm `cmake --preset <target>` succeeds from a clean clone.
