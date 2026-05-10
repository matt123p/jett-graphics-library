---
title: Build and License Guide
---

# Build and license guide

The repository contains two build surfaces:

- the native C++ library and test solution under `GraphicsLibrary.sln`
- the Jekyll documentation site under `docs/`

The published documentation is available at [matt123p.github.io/jett-graphics-library](https://matt123p.github.io/jett-graphics-library/).

## Native build requirements

The native codebase is currently a Windows-first Visual Studio solution. To build it from source you need:

- Git
- Visual Studio 2022 with the Desktop development with C++ workload
- the MSVC v143 toolset
- a recent Windows SDK
- PowerShell for the helper scripts and bootstrap commands
- an OpenCL runtime and driver if you want to run the GPU backend

The solution currently defines `Debug|Win32` and `Release|Win32` configurations. The repository-level MSBuild props file enables `vcpkg` manifest mode automatically and selects the `x86-windows` triplet for Win32 builds.

## Third-party dependencies

The native build uses the repository-local `vcpkg` checkout plus the manifest in `vcpkg.json` to restore these packages:

- FreeType
- Little CMS 2
- libjpeg-turbo with JPEG 8 compatibility enabled
- libpng
- libtiff
- zlib

## Build the native library

1. Open a Developer PowerShell for Visual Studio 2022, or another shell where `msbuild` is available on `PATH`.
2. Bootstrap the local `vcpkg` copy from the repository root:

```powershell
.\vcpkg\bootstrap-vcpkg.bat
```

3. Build the solution:

```powershell
msbuild .\GraphicsLibrary.sln /p:Configuration=Release /p:Platform=Win32
```

4. Swap `Release` for `Debug` when you need a debug build.
5. If you prefer the IDE, open `GraphicsLibrary.sln` in Visual Studio and build either the `GraphicsLibrary` project or the whole solution.

On the first build, `vcpkg` manifest mode restores the declared native dependencies automatically. If the `vcpkg/` directory is missing from your checkout, restore it before building because the solution expects a repository-local `vcpkg` root.

## Run the test executable

The `GraphicsTest` project builds a native test runner alongside the library. After building the solution, run the generated `GraphicsTest` executable from the solution output directory.

The test runner performs generated-asset checks, colour-management tests, CPU and GPU rendering passes, and output comparisons between the two execution modes. GPU-oriented tests require a working OpenCL stack. For CPU-only integration work, initialize the library with `jett::init(false)` instead of `jett::init(true)`.

## Build the documentation locally

The docs site is a standalone Jekyll project under `docs/`. To build or preview it locally you need Ruby and Bundler.

Install the gem dependencies and build the site with the repository helper script:

```powershell
.\GraphicsLibrary\build-docs-jekyll.ps1 -InstallDependencies
.\GraphicsLibrary\build-docs-jekyll.ps1
```

To serve the docs locally with live reload:

```powershell
.\GraphicsLibrary\build-docs-jekyll.ps1 -Serve
```

If you prefer to work directly inside `docs/`, the equivalent command is `bundle exec jekyll build`.

## Project license

The source code in this repository is distributed under the GNU Lesser General Public License, version 3.0 (LGPL-3.0). The full text ships in the top-level `LICENSE` file.

In practice, that means the library itself remains under LGPL-3.0 even when used by another application, and any redistribution of the library should preserve the required license notices and LGPL rights for recipients. Treat this section as a practical summary rather than legal advice.

For a more focused walkthrough of those terms, see [LGPL license](open-source-licenses/).