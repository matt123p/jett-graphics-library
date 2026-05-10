---
title: Jett Graphics Library
---

# Jett Graphics Library

The Jett Graphics Library is a high-speed 2D drawing library aimed at print-oriented image composition. It works with monochrome, RGB, CMYK, and CIE L*a*b* imagery; supports JPEG, PNG, BMP, and TIFF files; handles ICC colour management; and can execute key work on either the CPU or the GPU through OpenCL.

## Documentation map

- [Build and license guide](build-and-license-guide/) explains the Windows toolchain, `vcpkg` dependency bootstrap, local Jekyll docs workflow, and the overall LGPL-3.0 license.
- [BitBlt algorithms](bitblt-algorithms/) explains scaling, rotation, matrix transforms, transparency, and clipping.
- [Line drawing](line-drawing/) explains stroke construction, anti-aliasing, joins, and practical drawing choices.
- [Polygon algorithms](polygon-algorithms/) explains scanline filling, anti-aliasing, and line joins.
- [Colour management guide](colour-management-guide/) explains ICC profiles, rendering intents, and practical RGB/CMYK/Lab workflows.
- [Text rendering guide](text-rendering-guide/) explains font loading, glyph caching, rotation, and matrix-driven text effects.
- [Image I/O guide](image-io-guide/) explains in-memory image types, bit depth, embedded profiles, and file-format tradeoffs.
- [OpenCL](opencl/) explains what OpenCL is, why the GPU backend helps, and how to use cache policy and direct GPU access with this library.
- [Dithering](additional-features-guide/) covers dithering, linearization, and print-oriented finishing stages.
- [LGPL license](open-source-licenses/) explains the main LGPL-3.0 terms that apply to using and redistributing the library.

## API roadmap

The public API is centered on the types declared in `jett.h`:

- `jett` is the library context. It selects CPU or GPU execution, creates helper objects, and performs drawing, conversion, dithering, and text operations.
- `jett_image` owns image pixels, metadata, clipping state, embedded ICC profile data, and CPU/GPU cache policy.
- `jett_transform` is an opaque handle for colour conversion. Build it once and reuse it for repeated `bitblt()` or `convert()` calls.
- `jett_font` is an opaque handle for text rendering.
- `jett_screens` and `jett_linearization` are opaque handles used by the ordered dithering pipeline.
- `jett_matrix` and `jett_point` are lightweight geometry helpers used by matrix-based `bitblt()`, line drawing, polygon filling, and text placement.

In practice most applications follow this sequence:

1. Create an `jett` object and call `init()` once.
2. Load or create one or more `jett_image` objects.
3. Attach ICC profile information to images when colour conversion is required.
4. Build any long-lived resources such as transforms, fonts, screens, or linearization sets.
5. Compose the output image with `bitblt()`, `lines()`, `polygon()`, `rectangle()`, and `text()`.
6. Dither or linearize when preparing printer-ready output.
7. Save the final image or access raw pixel data.
8. Destroy opaque handles created by the `jett` object.

Built-in profile identifiers include `:srgb`, `:mono`, `:lab`, `:mono_cmyk`, and `:mono_rgb`.

If you are setting up a fresh development machine, start with [Build and license guide](build-and-license-guide/) before compiling examples or wiring the library into an application.

## Typical print workflow

The normal process for formatting an image for printing is:

1. Load an image into memory.
2. Convert the image into the printer's colour space using an ICC profile.
3. Rotate and resize the image to the printer's native resolution.
4. Dither or linearize the image to match the printer's grey-level capabilities.
5. Send the result to the printer or save it for later output.

If the image is variable data, the library helps compose the output from bitmaps, text, polygons, and lines directly in RGB or CMYK. That lets you keep colour control across the whole workflow instead of dropping into a screen-oriented graphics pipeline first.

## GPU acceleration

In high-throughput variable-data systems, the GPU can provide significant speedups over the CPU. The library uses OpenCL for graphics-intensive operations, and in the right workload it can be much faster than the CPU path. The main caveat is that the transfer of image data between CPU-visible memory and GPU memory can dominate the cost if cache modes are chosen poorly. A discrete desktop GPU is generally required before the GPU backend becomes worthwhile.

## Getting started

To use the library, create an `jett` object and initialize it for either CPU or GPU execution:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);
}
```

Starting on the CPU path is usually the better way to learn the API. It is easier to reason about and avoids hardware-specific OpenCL issues while you are still learning the object model.

You can use the CPU and GPU at the same time by creating two `jett` contexts, one per backend. Images and transforms can be passed between them when needed.

Here is a minimal line-drawing example:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image;
	image.createImage(250, 250, image_cmyk);
	image.erase();

	jett_point points[2];
	points[0] = jett_point(10, 10);
	points[1] = jett_point(240, 10);
	unsigned char cmyk_col[4] = {0, 255, 0, 0};
	r.lines(image, cmyk_col, 4, points, 2, false, 0);

	image.saveToFile(_T("output.tiff"));
}
```

<figure class="doc-figure">
	<img src="{{ '/images/overview-simple-example.png' | relative_url }}" alt="A simple magenta line rendered on a white background.">
	<figcaption>The generated starter example is intentionally simple: initialize the library, create an image, draw, and save.</figcaption>
</figure>

Most API failures throw `jett_exception`, so production code should wrap top-level operations in `try`/`catch` blocks. If you initialize the GPU backend on a machine without a working OpenCL stack, `init()` will throw.

## Drawing images

The basic building block of the library is `jett_image`. It represents a block of memory that the library can load, save, convert, and draw into.

The simplest image workflow is load, process, and save:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image;
	image.loadFromFile(_T("input.tiff"));
	image.saveToFile(_T("output.jpg"));
}
```

When an image is loaded or created it has a specific pixel format. That describes whether the image is monochrome, RGB, CMYK, or Lab; whether it has an alpha channel; and whether it uses Windows-oriented BGR ordering or RGB ordering.

### BitBlt

`bitblt()` is the central image-composition call. It can copy, crop, resize, rotate, colour-convert, alpha-blend, and clip during a single operation.

This example resizes an image by a factor of three:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image_in;
	image_in.loadFromFile(_T("input.tiff"));

	jett_image image_out;
	image_out.createImage(image_in.getWidth() * 3, image_in.getHeight() * 3, image_in.getType());

	r.bitblt(NULL, image_in, 0, 0, image_in.getWidth(), image_in.getHeight(),
			 image_out, 0, 0, image_out.getWidth(), image_out.getHeight(), bitblt_cubic_scaling);

	image_out.saveToFile(_T("output.tiff"));
}
```

Converting between compatible pixel formats such as RGB and BGR can be done with `bitblt()` alone. Converting between incompatible spaces such as RGB and CMYK requires an ICC transform:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image_in;
	image_in.loadFromFile(_T("input.tiff"));

	jett_image image_out;
	image_out.createImage(image_in.getWidth(), image_in.getHeight(), image_cmyk);
	image_out.set_profile_data(_T("USWebCoatedSWOP.icc"));

	jett_transform t = r.build_transform(_T(":srgb"), image_out, INTENT_PERCEPTUAL);
	r.bitblt(t, image_in, 0, 0, image_in.getWidth(), image_in.getHeight(),
			 image_out, 0, 0, image_out.getWidth(), image_out.getHeight(), 0);

	image_out.saveToFile(_T("output.tiff"));
	r.destroy_transform(t);
}
```

The library can also load, save, and draw into Lab TIFF images. For a deeper explanation of how the copy path works, see [BitBlt algorithms](bitblt-algorithms/).

### Composition and alpha blending

The same call can combine resize, rotation, and colour conversion in one step. That makes `bitblt()` the backbone of multi-image composition workflows.

All drawing primitives, including image copies, support alpha blending, including in CMYK mode. Matrix-based `bitblt()` uses `jett_matrix` to rotate or otherwise transform an image at arbitrary angles rather than only quarter turns.

### CPU and GPU cache modes

An image may live in CPU memory or GPU memory. The library automatically moves or copies it as necessary before an operation, but that movement can be expensive. Choosing the right image cache mode is therefore a practical performance decision rather than a minor implementation detail. The dedicated [OpenCL](opencl/) page covers the GPU backend, cache modes, and direct `cl_mem` access in more detail.

<div class="doc-gallery doc-gallery--triple">
	<figure class="doc-figure">
		<img src="{{ '/images/bitblt-composition.png' | relative_url }}" alt="Three composited icons placed on a white background.">
		<figcaption>Composition example using repeated <code>bitblt()</code> calls.</figcaption>
	</figure>
	<figure class="doc-figure">
		<img src="{{ '/images/bitblt-alpha.png' | relative_url }}" alt="Translucent icons blended over a colorful photographic background.">
		<figcaption>Alpha blending works cleanly with anti-aliased source imagery.</figcaption>
	</figure>
	<figure class="doc-figure">
		<img src="{{ '/images/bitblt-matrix.png' | relative_url }}" alt="Several copies of the same icon arranged around a circle using a matrix transform.">
		<figcaption>Matrix-driven placement handles arbitrary rotation and repeated transformed copies.</figcaption>
	</figure>
</div>

## Drawing lines and polygons

The line API can draw single segments, polylines, or closed shapes. Join style can be bevel, miter, or none.

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image;
	image.createImage(250, 250, image_cmyk);
	image.erase();

	jett_point points[4];
	points[0] = jett_point(25, 25);
	points[1] = jett_point(25, 225);
	points[2] = jett_point(225, 225);
	points[3] = jett_point(225, 25);

	unsigned char cmyk_col[4] = {0, 255, 0, 0};
	r.lines(image, cmyk_col, 20, points, 4, false, line_join_bevel);

	image.saveToFile(_T("output.tiff"));
}
```

Closed polylines can become polygons by setting the `close` parameter to `true`. Filled polygons use the `polygon()` API and can be rendered with or without anti-aliasing. The algorithm-level details are in [Polygon algorithms](polygon-algorithms/).

## Drawing text

Text can be drawn either from a Windows `HFONT` or through the internal FreeType-based font path. The FreeType route is the more flexible option when you need anti-aliased text or matrix transforms.

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(false);

	jett_image image;
	image.createImage(250, 250, image_cmyk);
	image.erase();

	unsigned char colour[4] = {192, 64, 0, 0};
	jett_font font = r.create_font(_T("C:\\Windows\\Fonts\\Arial.ttf"), 0, 20, true);

	r.text(font, colour, image, 125, 125, "Text at 0", string_rotate_0);
	r.text(font, colour, image, 125, 125, "Text at 90", string_rotate_90);
	r.text(font, colour, image, 125, 125, "Text at 180", string_rotate_180);
	r.text(font, colour, image, 125, 125, "Text at 270", string_rotate_270);

	image.saveToFile(_T("output.tiff"));
	r.destroy_font(font);
}
```

Font matrices work with the FreeType engine and let you rotate or distort text at arbitrary angles. For the underlying rendering model, see [Text rendering guide](text-rendering-guide/).

Barcode support is not included in this build.

## Clipping

Each `jett_image` carries a clip rectangle. The clip rectangle is used when the image is the destination of drawing, not when it is the source of a `bitblt()`.

Setting a new clip rectangle replaces the previous one; clip rectangles are not combined. The library also clamps the clip rectangle to the image bounds.

## Colour management

Every image has an associated colour space, either from an embedded ICC profile or from a built-in assumption such as sRGB. A colour transform joins the source and destination profiles so the library knows how to move colour meaning from one image into another.

In practice:

1. make sure the source and destination profiles are defined,
2. build a transform,
3. reuse it for repeated `bitblt()` or `convert()` calls,
4. destroy it when you are finished.

For the full explanation of rendering intents, built-in profiles, and RGB/CMYK/Lab workflows, see [Colour management guide](colour-management-guide/).

## Dithering

Inkjet printers usually cannot reproduce a full continuous tone at every pixel, so the library uses ordered dithering to convert continuous-tone images into printer-friendly output. The implementation uses blue-noise style screens generated through a void-and-cluster process.

Practical guidance from the original docs still applies:

- start with screen sizes of at least 32 by 32,
- avoid using the same size for every plane,
- choose the sigma factor based on screen size,
- save generated screens for reuse in production.

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(true);

	int sizes[] = {32, 33, 34, 35};
	jett_screens screens = r.create_screens(4, sizes, 1.5, 2);

	jett_image rgb_in;
	rgb_in.loadFromFile("Lenna.png");

	jett_image cmyk_out;
	cmyk_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_cmyk);
	cmyk_out.set_profile_data("USWebCoatedSWOP.icc");

	jett_transform t = r.build_transform(":srgb", cmyk_out, INTENT_PERCEPTUAL);
	r.bitblt(t, rgb_in, 0, 0, rgb_in.getWidth(), rgb_in.getHeight(),
			 cmyk_out, 0, 0, cmyk_out.getWidth(), cmyk_out.getHeight(), 0);

	jett_image outputs[4];
	outputs[0].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[1].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[2].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[3].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);

	r.dither(cmyk_out, outputs, screens);

	outputs[0].saveToFile("output_C.bmp");
	outputs[1].saveToFile("output_M.bmp");
	outputs[2].saveToFile("output_Y.bmp");
	outputs[3].saveToFile("output_K.bmp");
}
```

The broader feature context is covered in [Additional features guide](additional-features-guide/).

## Linearization

Linearization compensates for the fact that real printers do not respond linearly to requested ink levels. In this library it is normally part of the dithering stage.

The original guidance remains sound: the best way to determine a linearization curve is to measure output with a spectrophotometer, although rough estimation by densitometer or by eye can be useful during development.

The following example creates tone patches, saves them once without linearization, then applies a linearization curve and saves them again:

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(true);

	jett_image image_out;

	jett_point points[5] = {
		jett_point(0, 0),
		jett_point(90, 56),
		jett_point(150, 40),
		jett_point(200, 100),
		jett_point(255, 200)
	};

	jett_linearization linearization = r.create_linearization();
	r.import_linearization(linearization, points, 5, false, false);
	r.import_linearization(linearization, points, 5, false, false);
	r.import_linearization(linearization, points, 5, false, false);
	r.import_linearization(linearization, points, 5, false, false);

	int sizes[] = {32, 33, 34, 35};
	jett_screens screens = r.create_screens(4, sizes, 1.5, 256);

	image_out.saveToFile("No Linearization.tiff");
	r.dither(image_out, screens, linearization);
	image_out.saveToFile("With Linearization.tiff");
}
```

## GDI and OpenCL integration

If you need to integrate the library with existing Windows drawing code, `jett_image::createBitmap` can create an image backed by an `HBITMAP`. That lets you draw with GDI or GDI+ and then continue processing the result through the library, including colour conversion and alpha-aware composition.

For OpenCL integration, advanced users can lock image data directly in GPU memory, pass the resulting `cl_mem` object to their own kernels, and then unlock the image when processing is complete. This is useful, but it only pays off when you avoid unnecessary transfers between CPU and GPU memory. The [OpenCL](opencl/) page covers that workflow in full.

## Example outputs

The checked-in example output files live under `example images/cpu` and `example images/gpu`. The historical Doxygen page referenced PNG derivatives of those examples, but the repository currently stores the TIFF and BMP output files instead.

