---
title: BitBlt Algorithms
---

# How BitBlt, scaling, and matrices work

The `bitblt` family is the library's main image-composition tool. If polygons and text create pixels, `bitblt` decides how one image is copied, transformed, colour-converted, and merged into another image.

## What a bitblt does

At the simplest level, a `bitblt` copies pixels from a source image into a destination image.

In this library the same call can also:

- crop a source rectangle,
- place it at a new destination position,
- resize it,
- rotate it in 90 degree steps,
- apply an arbitrary affine matrix,
- convert between colour spaces using an ICC transform,
- treat white as transparent for greyscale mask-style work,
- blend using an alpha channel,
- respect the destination clip rectangle.

## The normal rectangular pipeline

The rectangle-based overloads of `jett::bitblt()` broadly work like this:

1. Choose the destination pixel being written.
2. Map that pixel back to the corresponding source position.
3. Decide how to sample the source position.
4. Optionally run the sampled colour through a colour transform.
5. Optionally blend with the destination pixel.
6. Write the result.

The key mental model is to walk over the destination and ask where each destination pixel should read from in the source.

## Scaling

### Nearest-neighbour scaling

`bitblt_nn_scaling` is the default. It picks the nearest source pixel and copies it.

Advantages:

- very fast,
- preserves hard pixel edges,
- ideal for masks, previews, and already-dithered imagery.

Drawbacks:

- enlarged images look blocky,
- reduced images can shimmer or alias.

### Cubic scaling

`bitblt_cubic_scaling` samples a 4 by 4 neighbourhood and uses cubic interpolation to estimate a smoother value.

Advantages:

- smoother enlargement,
- better detail retention when resizing photographs and artwork,
- more natural-looking composition when placing source art at a different size.

Drawbacks:

- slower than nearest-neighbour,
- near image edges the implementation falls back to a simpler sample when there is not a full neighbourhood available.

## Rotation

The flag-based rotation path supports `bitblt_rotate_0`, `bitblt_rotate_90`, `bitblt_rotate_180`, and `bitblt_rotate_270`.

These are exact quarter turns. They are efficient and ideal for label, page, and print layout work where the asset only needs to be turned upright or sideways.

If you need an arbitrary angle, use the matrix overload instead.

## Matrix-based bitblt

The matrix overload of `jett::bitblt()` lets you apply a full affine transform. The matrix stored by `jett_matrix` is:

```text
x' = a*x + c*y + e
y' = b*x + d*y + f
```

This means:

- `e` and `f` translate the image,
- `a` and `d` control scale when the off-diagonal terms are zero,
- `b` and `c` introduce rotation or shear depending on the combination.

Affine transforms matter because one matrix can combine multiple effects without producing intermediate images.

## Matrix composition and inversion

`jett_matrix::append()` multiplies the current matrix by another one. In practice the order matters: rotating and then translating is not the same as translating and then rotating.

`jett_matrix::invert()` returns the inverse transform when one exists. If the matrix collapses area, for example by scaling one axis to zero, inversion fails and the library throws `JETT_MATRIX_CANNOT_INVERT`.

## Colour transforms during bitblt

If you pass a non-null `jett_transform`, `bitblt` converts the sampled source pixel into the destination colour space while copying.

That means one call can resize an RGB image, convert it into CMYK with an ICC profile, and place it into a destination layout in one pass.

<figure class="doc-figure">
	<img src="{{ '/images/bitblt-composition.png' | relative_url }}" alt="Three converted icons composed into a CMYK destination image.">
	<figcaption>BitBlt is not just a copy. The same pass can place, convert, and merge source assets into the destination layout.</figcaption>
</figure>

## Transparency and alpha

### White-as-transparent

`bitblt_white_is_transparent` treats white source pixels as empty background while darker values act like coverage. This is useful for mask-style monochrome overlays.

### Alpha blending

`bitblt_use_alpha` uses the source alpha channel when the source image has one. This is the normal choice for PNG-style compositing where soft edges need to blend into the destination.

<figure class="doc-figure">
	<img src="{{ '/images/bitblt-alpha.png' | relative_url }}" alt="Soft-edged icons blended over a vivid photographic background using alpha.">
	<figcaption>Alpha blending preserves soft edges, which is especially important for anti-aliased source graphics.</figcaption>
</figure>

## Clipping

`bitblt` respects the clip rectangle of the destination image. Clipping is a property of where you draw into, not where you read from.

<figure class="doc-figure">
	<img src="{{ '/images/clipping-example.png' | relative_url }}" alt="A translucent shape clipped so only part of it appears in the destination image.">
	<figcaption>Destination clipping constrains where the copy can land, regardless of the size of the source asset.</figcaption>
</figure>

## Choosing the right overload

Use the simple overloads when you want to copy a whole source image, place it at one location, resize into a destination rectangle, or do quarter-turn rotation.

Use the matrix overload when you want arbitrary rotation, combined scale and rotation, shear, or placement through a transform you already share with text or geometry.

<figure class="doc-figure">
	<img src="{{ '/images/bitblt-matrix.png' | relative_url }}" alt="Multiple rotated copies of the same icon arranged in a ring with a matrix transform.">
	<figcaption>The matrix overload becomes the right tool as soon as you need anything more expressive than quarter turns.</figcaption>
</figure>

## Common beginner patterns

- Photo into print layout: cubic scaling plus a colour transform.
- Logo with transparent background: alpha blending plus an optional colour transform.
- Simple mask overlay: white-is-transparent.
- Rotated product icon: matrix `bitblt`.

## Related pages

- [Home](./../)
- [Colour management guide](../colour-management-guide/)
- [Text rendering guide](../text-rendering-guide/)
