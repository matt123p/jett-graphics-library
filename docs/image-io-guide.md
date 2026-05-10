---
title: Image I/O Guide
---

# Loading, saving, image types, and file formats

This page explains the difference between the library's in-memory image types and the disk formats used to load and save them.

That distinction matters because:

- `image_t` describes how pixels are stored while you work on them in memory.
- JPEG, PNG, BMP, and TIFF describe how pixels are stored on disk.

## In-memory image types

### Low-bit monochrome outputs

- `image_mono1`
- `image_mono2`
- `image_mono4`

These are primarily printer-style destination formats for dithering output.

### Full monochrome

`image_mono` stores one greyscale channel per pixel and can be 8-bit or 16-bit when created in memory.

### RGB and BGR

- `image_rgb`
- `image_bgr`

These store the same kind of colour data but in different channel order.

### RGBA and BGRA

- `image_rgba`
- `image_bgra`

These add alpha for transparency and are ideal for compositing.

### CMYK

`image_cmyk` stores four subtractive channels and is natural for print workflows.

### Lab

`image_lab` stores CIE L*a*b* colour, which is useful for colour management and interchange.

## Bit depth

When you create an image in memory you can choose 8-bit or 16-bit channels for the main full-colour formats.

Higher bit depth gives you more precision and smoother gradients, but it costs more memory, bandwidth, and processing time.

## Supported file formats

### TIFF

TIFF is the most capable format in this library. It supports monochrome, RGB, RGBA, CMYK, and Lab output; embedded ICC profiles; and both 8-bit and 16-bit channel data.

### PNG

PNG is good for lossless RGB-oriented assets and transparent overlays. In this library, 16-bit PNG input is reduced to 8-bit on load, and CMYK/Lab save paths are not supported.

### JPEG

JPEG is a compact photographic exchange format. It is useful for distribution but not ideal as a repeatedly edited master format.

### BMP

BMP is the simplest supported format. It is useful for debugging and Windows interoperability, but it is not the strongest choice for serious interchange.

## Embedded ICC profiles

Where the file format supports it, the library imports embedded ICC profiles on load and can write profile data on save.

## Resolution metadata

Images also carry DPI metadata. This does not change the pixels; it tells downstream systems how large those pixels are intended to be physically.

## Choosing a format

Choose TIFF when you need CMYK, Lab, 16-bit precision, or robust production storage.

Choose PNG when you need alpha transparency and the asset is RGB-oriented.

Choose JPEG when the source is photographic and file size matters more than exact pixel preservation.

Choose BMP when you need a simple compatibility or debugging format.

## Related pages

- [Home](./../)
- [Colour management guide](../colour-management-guide/)
- [Additional features guide](../additional-features-guide/)
