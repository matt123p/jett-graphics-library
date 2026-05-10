# Jett Graphics Library

Jett Graphics Library is a high-speed 2D drawing library aimed at print-oriented image composition. It supports monochrome, RGB, CMYK, and Lab imagery; ICC colour management; text, line, polygon, and image composition; and CPU or OpenCL-backed execution.

Documentation is published at [matt123p.github.io/jett-graphics-library](https://matt123p.github.io/jett-graphics-library/).

## Build requirements

- Windows
- Visual Studio 2022 with Desktop development with C++
- MSVC v143 and a recent Windows SDK
- the repository-local `vcpkg` checkout for native dependencies
- Ruby and Bundler if you want to build the Jekyll documentation locally
- an OpenCL runtime if you want to exercise the GPU backend

## Build the native solution

Bootstrap `vcpkg`, then build the Win32 solution:

```powershell
.\vcpkg\bootstrap-vcpkg.bat
msbuild .\GraphicsLibrary.sln /p:Configuration=Release /p:Platform=Win32
```

The solution uses `vcpkg` manifest mode, so the declared packages are restored automatically during the build. Open `GraphicsLibrary.sln` in Visual Studio if you prefer building from the IDE.

## Build the documentation site

```powershell
.\GraphicsLibrary\build-docs-jekyll.ps1 -InstallDependencies
.\GraphicsLibrary\build-docs-jekyll.ps1 -Serve
```

That script builds the Jekyll site in `docs/` and can also serve it locally for preview.

## License

Jett Graphics Library is licensed under the GNU Lesser General Public License v3.0. See `LICENSE` for the full text.

The published documentation also includes an LGPL overview page at `docs/open-source-licenses.md`.