# Repository Guidelines

## Project Structure & Module Organization
Frame's engine code lives in `src/frame` with public headers mirrored under `include/frame`. Runtime assets (models, textures, shaders) reside in `asset/`; keep generated artifacts out of Git. `examples/` holds runnable scenes that double as integration samples. `tests/frame` contains GoogleTest suites; subfolders such as `json/` and `opengl/` isolate feature coverage. CMake helpers live under `cmake/`, and `external/` carries the VCPKG submodule fetched during configure. All build output lands in `build/<preset>`; do not commit that tree.

## Build, Test, and Development Commands
Run `git submodule update --init --recursive` after cloning to hydrate `external/`. Configure with `cmake --preset linux-debug` (or `linux-release`, `windows`) to create a Ninja or Visual Studio tree in `build/...`. Build via `cmake --build --preset linux-debug`. To smoke-test an example, execute `./build/linux-debug/examples/frame_viewer` once the preset is built. For quick test rebuilds, run `cmake --build --preset linux-debug --target FrameTest`.

## Coding Style & Naming Conventions
C++ sources follow `.clang-format` (Microsoft base, 4-space indent, 80-column limit, left-aligned pointers); run `clang-format -i path/to/file.cpp` before committing. `.editorconfig` enforces CRLF line endings and trims trailing whitespace. Folders mirror namespaces (e.g., `frame/opengl`), filenames use snake_case, and tests end with `_test.cpp`. Public classes use PascalCase, whereas functions and variables stay camelCase; prefer `kPrefix` for constants.

## Testing Guidelines
Unit tests rely on GoogleTest under `tests/frame`. Name suites after the component (`CameraTest`) and keep individual tests in `Method_State_Expectation` form. After building, run `ctest --test-dir build/linux-debug --output-on-failure`; point to the matching build directory for other presets. Ship new features with targeted tests or a validated example update. For GPU changes, add or refresh an `examples/` scene if automation is impractical.

## Commit & Pull Request Guidelines
Commits use an imperative summary (e.g., `Update material parser defaults`) under 72 characters, mirroring the current history. Group related changes and avoid WIP checkpoints. Pull requests should describe intent, list the build or test commands executed, and link the relevant issue. Include before/after captures whenever rendering, assets, or editor UX changes. Tag reviewers responsible for the subsystem touched to keep turnaround tight.

## Asset & Dependency Notes
Large binaries belong in `asset/` or managed external storage; coordinate before adding anything over ~10 MB. When adjusting VCPKG ports or submodules, note the change in the PR and confirm `cmake --preset <target>` still succeeds from a clean clone.
