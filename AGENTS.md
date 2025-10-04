# Repository Guidelines

## Project Structure & Module Organization
- Core engine sources sit in src/frame, with public headers mirrored in include/frame for external consumption.
- Runtime assets (models, textures, shaders) belong in sset/; keep generated or temporary files out of git.
- Integration samples live in xamples/, while unit tests reside under 	ests/frame/<feature>/ to match the component under test.
- Build products are written to uild/<preset>; clean this tree before commits to avoid shipping artifacts.

## Build, Test, and Development Commands
- git submodule update --init --recursive: syncs all xternal/ dependencies after cloning or pulling.
- cmake --preset linux-debug (or linux-release, windows): configures the build directory with the chosen toolchain and options.
- cmake --build --preset linux-debug: incrementally compiles the project; append --target FrameTest to focus on a test binary.
- ./build/linux-debug/examples/frame_viewer: launches the sample viewer once binaries have been produced.

## Coding Style & Naming Conventions
- Follow the repo .clang-format (Microsoft base, 4 spaces, 80-column limit, left-aligned pointers); run clang-format -i on touched files.
- Respect .editorconfig: CRLF endings and trimmed trailing whitespace.
- Use PascalCase for public classes, camelCase for functions and variables, and kName for constants; align filenames with namespaces using snake_case.

## Testing Guidelines
- GoogleTest powers the suite; place tests beside their feature in 	ests/frame/<feature>/ with ComponentTest fixture names.
- Name cases with the Method_State_Expectation convention for clarity.
- Execute ctest --test-dir build/linux-debug --output-on-failure after building to run the suite; adjust the preset directory as needed.

## Commit & Pull Request Guidelines
- Write imperative commit messages under 72 characters (e.g., Update material parser defaults) and group related changes.
- PRs should explain intent, list exercised build/test presets, and link relevant issues.
- Provide before/after renders for graphics-facing updates and flag subsystem reviewers when applicable.

## Configuration & Assets
- Avoid adding binaries larger than ~10 MB without coordinating with maintainers.
- Call out any changes to VCPKG submodules or ports in xternal/, and confirm a clean cmake --preset <target> build.
- Keep sset/ organized; do not commit generated shaders or other build outputs.
