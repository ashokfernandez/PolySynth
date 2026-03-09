# Multi-Platform Build System Sync Guide

This project maintains **three parallel build systems** that MUST stay in sync.
Forgetting to update one causes CI failures that are painful to debug because
the release builds run on remote runners (macOS + Windows) with no local
reproducer.

## Build System Map

| Build System | Platforms | Key Config Files |
|---|---|---|
| **CMake** | Tests, ComponentGallery, desktop (dev) | `tests/CMakeLists.txt`, `ComponentGallery/CMakeLists.txt`, `src/platform/desktop/CMakeLists.txt` |
| **Xcode (xcodebuild)** | macOS release (AU, VST3, APP) | `src/platform/desktop/config/PolySynth-mac.xcconfig`, `projects/PolySynth-macOS.xcodeproj/project.pbxproj` |
| **MSBuild (vcxproj)** | Windows release (VST3) | `src/platform/desktop/config/PolySynth-win.props`, `projects/PolySynth-vst3.vcxproj` |

## Checklists

### Adding a new `.cpp` source file

1. **CMake**: Add to the `SOURCES` list in the relevant `CMakeLists.txt`
2. **Xcode**: Add to the pbxproj compile sources (open in Xcode or edit `project.pbxproj`)
3. **Windows vcxproj**: Add a `<ClCompile Include="..."/>` entry in `PolySynth-vst3.vcxproj`
   - Paths are relative to the `projects/` directory
   - Repo root = `..\..\..\..\` from projects/

### Adding a new include directory

1. **CMake**: `include_directories(...)` in the relevant CMakeLists.txt
2. **Xcode xcconfig**: Append to `EXTRA_INC_PATHS` in `PolySynth-mac.xcconfig`
   - `$(SRCROOT)` = the `projects/` directory, so repo root = `$(SRCROOT)/../../../../`
3. **Windows props**: Append to `<AdditionalIncludeDirectories>` in `PolySynth-win.props`
   - `$(ProjectDir)` = the `projects/` directory, so repo root = `$(ProjectDir)..\..\..\..\`
   - Keep the closing `</AdditionalIncludeDirectories>` tag intact

### Adding new resources (fonts, images, presets)

1. **Xcode**: Add to the "Copy Bundle Resources" build phase in the pbxproj
2. **Windows**: Ensure the post-build copy scripts handle the new resource
3. **CMake**: Update install/copy targets if applicable

### Adding a new library dependency

1. Update all three build systems' include paths AND linker settings
2. If the dependency needs downloading, add it to `scripts/download_dependencies.sh`
3. For Skia/iPlug2 prebuilt libs, ensure the CI workflow calls `download-prebuilt-libs.sh`

## CI-Specific Notes

### Windows CI builds without a .sln file

The `.sln` is gitignored. CI builds the `.vcxproj` directly using msbuild.
When doing so, `$(SolutionDir)` defaults to the vcxproj directory instead of
`src/platform/desktop/`. The CI workflow passes it explicitly:

```
msbuild ... "-p:SolutionDir=${{ github.workspace }}\src\platform\desktop\"
```

### Platform API differences

`BundleResourcePath` takes `PluginIDType` which varies per platform:
- **macOS** (`OS_MAC`): `const char*` — pass a bundle ID string
- **Windows** (`OS_WIN`): `HMODULE` — pass `gHINSTANCE`
- **Web**: not available

Always use `#if defined(OS_MAC)` / `#elif defined(OS_WIN)` / `#endif`.
Do NOT use `#ifndef WEB_API` alone — it doesn't distinguish macOS from Windows.

## Validation

Run `just ci-pr` locally before pushing. The test-release-builds workflow
(`gh workflow run test-release-builds.yml`) tests just the macOS + Windows
release jobs without the full CI pipeline.
