# Jett Graphics Library

Jett Graphics Library is a high-speed 2D drawing library aimed at print-oriented image composition. It supports monochrome, RGB, CMYK, and Lab imagery; ICC colour management; text, line, polygon, and image composition; and CPU or OpenCL-backed execution.

Documentation is published at [matt123p.github.io/jett-graphics-library](https://matt123p.github.io/jett-graphics-library/).

## Build requirements

### Windows

- Visual Studio 2022 with Desktop development with C++
- MSVC v143 and a recent Windows SDK
- the repository-local `vcpkg` checkout for native dependencies
- Ruby and Bundler if you want to build the Jekyll documentation locally
- an OpenCL runtime if you want to exercise the GPU backend

### Linux

- CMake 3.20+
- a C++17 compiler such as GCC 13 or Clang 16+
- development packages for FreeType, Little CMS 2, libjpeg, libpng, libtiff, and OpenCL headers/runtime
- `pkg-config`

On Ubuntu 24.04, the following installs the required native packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
	libfreetype-dev liblcms2-dev libjpeg-dev libpng-dev libtiff-dev \
	ocl-icd-opencl-dev
```

## Build the native solution

Bootstrap `vcpkg`, then build the Win32 solution:

```powershell
.\vcpkg\bootstrap-vcpkg.bat
msbuild .\GraphicsLibrary.sln /p:Configuration=Release /p:Platform=Win32
```

The solution uses `vcpkg` manifest mode, so the declared packages are restored automatically during the build. Open `GraphicsLibrary.sln` in Visual Studio if you prefer building from the IDE.

## Build on Linux

Configure and build the shared library with CMake:

```bash
cmake -S . -B build -DJETT_BUILD_TESTS=OFF
cmake --build build -j"$(nproc)"
```

This produces `build/libjett.so`.

Notes:

- The Linux build keeps the public FreeType-based font-loading API and the OpenCL backend.
- Windows-only GDI bitmap and HFONT entry points remain available only when building on Windows.
- `GraphicsTest` is disabled by default on Linux because the legacy test harness still assumes Windows-style assets and filesystem behavior.

## Build the documentation site

```powershell
.\GraphicsLibrary\build-docs-jekyll.ps1 -InstallDependencies
.\GraphicsLibrary\build-docs-jekyll.ps1 -Serve
```

That script builds the Jekyll site in `docs/` and can also serve it locally for preview.

## License

Jett Graphics Library is licensed under the GNU Lesser General Public License v3.0. See `LICENSE` for the full text.

The published documentation also includes an LGPL overview page at `docs/open-source-licenses.md`.