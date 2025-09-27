# Repository Guidelines

## Project Structure & Module Organization
Frame engine sources live in `src/frame`, with their public headers mirrored under `include/frame`. Runtime assets such as models, textures, and shaders belong in `asset/`; keep generated or temporary artifacts out of version control. Integration samples run from `examples/`, while unit tests live in `tests/frame/<feature>/`. Build products stay in `build/<preset>`; clean this tree before committing changes.

## Build, Test, and Development Commands
Run `git submodule update --init --recursive` after cloning to pull `external/` dependencies. Configure a local build with `cmake --preset linux-debug` (switch to `linux-release` or `windows` when needed). Compile iteratively via `cmake --build --preset linux-debug`, or rebuild a target such as `FrameTest` by adding `--target FrameTest`. Launch the sample viewer with `./build/linux-debug/examples/frame_viewer` once binaries are in place.

## Coding Style & Naming Conventions
C++ sources follow the repo `.clang-format` (Microsoft base, 4-space indent, 80-column limit, left-aligned pointers). Public classes use PascalCase; functions, variables, and private fields stay camelCase; constants take the `kName` form. Align filenames with namespaces using snake_case. Run `clang-format -i path/to/file.cpp` and honor `.editorconfig` rules (CRLF endings, trimmed whitespace).

## Testing Guidelines
All tests use GoogleTest in `tests/frame`. Name suites after the component (for example `CameraTest`) and cases as `Method_State_Expectation`. After building, execute `ctest --test-dir build/linux-debug --output-on-failure` to validate the full suite; adjust the preset path when switching configurations.

## Commit & Pull Request Guidelines
Write commit messages in imperative mood under 72 characters (e.g., `Update material parser defaults`). Group related changes and avoid WIP commits. Pull requests must explain intent, list the build or test presets exercised, and link related issues. Include before/after renders for graphics-facing changes and flag subsystem reviewers when relevant.

## Configuration & Assets
Coordinate before adding binaries larger than ~10 MB. When touching VCPKG submodules or ports in `external/`, note it in your PR and confirm a clean `cmake --preset <target>` succeeds. Keep `asset/` organized and avoid committing generated shaders or other build outputs.
