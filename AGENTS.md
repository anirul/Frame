# Repository Guidelines

## Project Structure & Module Organization
- Engine sources live in src/frame, with mirrored public headers in include/frame.
- Rendering backends sit under src/frame/<api> (for example src/frame/vulkan), while supporting tests mirror that layout beneath 	ests/frame/<feature>/.
- Runtime assets (sset/) include JSON scenes, shaders, and textures; keep generated artefacts and build outputs out of version control.
- Examples build from xamples/, with intermediate files landing in uild/<preset>; clean that tree before commits.

## Build, Test, and Development Commands
- git submodule update --init --recursive: fetches xternal/ dependencies after cloning or rebasing.
- Configure with cmake --preset windows (MSVC), cmake --preset linux-debug, or the release variants; customise with --fresh when toolchains change.
- Incremental builds use cmake --build --preset <preset>; add --target FrameVulkan or any other library/executable to focus compilation.
- Run examples from uild/<preset>/bin/<Example>.exe (Windows) or the matching ELF under uild/<preset>/examples/ on Linux.

## Coding Style & Naming Conventions
- Apply the repository .clang-format (Microsoft style, 4-space indent, 80-column soft limit) to all C++ changes; stick with CRLF endings per .editorconfig.
- Public types use PascalCase, functions and locals camelCase, constants kName, and filenames align with namespaces in snake_case.

## Testing Guidelines
- Primary targets: FrameTest, FrameOpenGLTest, FrameVulkanTest, plus JSON/file suites; build them with cmake --build --preset <preset> --target <TestTarget>.
- Execute ctest --test-dir build/<preset> --output-on-failure -C Debug (or Release) before sending patches; this runs the full GoogleTest suite, including Vulkan fixtures under 	ests/frame/vulkan.
- New tests should follow the fixture-per-component style used in OpenGL/Vulkan suites and live beside the feature they exercise.

## Commit & Pull Request Guidelines
- Use imperative, <72 character commit messages (e.g. Enable Vulkan window test).
- PR descriptions must state intent, list exercised presets/tests, and link issues; attach before/after renders for graphics changes when possible.
- Call out modifications to xternal/ or build scripts, and confirm clean configure/build on the affected presets.

## Configuration & Assets
- Coordinate before adding assets >10?MB.
- Do not commit generated shaders, compiled binaries, or build directories; update .gitignore if new tools emit artefacts.
- Maintain logical subfolders in sset/ (textures, materials, scenes) to keep example content manageable.
