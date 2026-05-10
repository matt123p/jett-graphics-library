---
title: Colour Management Guide
---

# How colour management works

Colour management can feel abstract when you first meet it, so this page starts from first principles and connects those ideas to the Graphics Library API.

## Why colour management exists

The numbers inside an image are not colours by themselves. They are instructions that only make sense relative to a colour space.

For example:

- in one RGB space, `255,0,0` is one particular red,
- in another RGB space, the same triplet can represent a slightly different red,
- in CMYK, four numbers describe ink amounts rather than emitted light.

Colour management translates colour meaning from one space into another as reliably as possible.

## The colour spaces in this library

### Monochrome

Monochrome is a single-channel intensity space used for masks, greyscale workflows, and printer pipelines where each output plane is handled separately.

### RGB and BGR

RGB is the natural space for screens, cameras, and general-purpose image assets. The library supports both `image_rgb` and `image_bgr`; they differ in channel order, not in the underlying model.

### CMYK

CMYK is a subtractive colour model that maps naturally to many print workflows because it stays close to the printer's real output model.

### CIE L*a*b*

Lab is a device-independent space that is useful as a connection space inside ICC workflows. The library exposes this as `image_lab`.

## Profiles and transforms

An ICC profile describes a colour space. A colour transform describes the path from one profile to another.

In this library:

- an `jett_image` may carry embedded profile data,
- you can attach profile data explicitly with `jett_image::set_profile_data()`,
- you build a reusable `jett_transform` with `jett::build_transform()`,
- you use that transform in `jett::bitblt()` or `jett::convert()`.

Building a transform is relatively expensive. Reusing it is efficient.

## Custom transform samplers

Not every useful mapping is most naturally expressed as an ICC profile. For those cases the library exposes `transform_sampler` so you can provide your own sampling logic, build a transform from it, and still use that transform through the normal colour-conversion path.

This is useful for:

- special device mappings,
- experimental transforms,
- proofing logic that sits outside a conventional ICC workflow,
- interpolation-based conversions where a profile is not the most natural representation.

The important point is that this feature still fits into the same mental model as the rest of the page: build a transform once, then reuse it through `bitblt()` or `convert()`.

## Built-in profiles

The library provides built-in profile tokens for common cases:

- `:srgb`
- `:mono`
- `:lab`
- `:mono_cmyk`
- `:mono_rgb`

These are convenient when you want a known transform without shipping a separate profile file for every test program.

## Rendering intents

When two colour spaces do not cover the same gamut, the rendering intent tells the colour-management system how to make the compromise.

### `INTENT_PERCEPTUAL`

Compress the whole gamut so the image keeps an overall natural appearance. This is often the safest choice for photographs.

### `INTENT_RELATIVE_COLORIMETRIC`

Map the white point and preserve in-gamut colours more strictly while clipping out-of-gamut colours.

### `INTENT_SATURATION`

Prefer vividness over strict colour accuracy. This is more common in business graphics than in photographic reproduction.

### `INTENT_ABSOLUTE_COLORIMETRIC`

Preserve absolute colour values including paper white differences. This is typically used for proofing.

## Black-preservation intents

The library also exposes CMYK-specific intents that preserve black in different ways:

- `INTENT_PRESERVE_K_ONLY_PERCEPTUAL` and related variants preserve the K channel itself.
- `INTENT_PRESERVE_K_PLANE_PERCEPTUAL` and related variants preserve the K plane.

These options matter in print workflows where black ink behaviour affects stability, cost, or visual consistency.

## Practical thinking

Use RGB when assets come from screens or cameras and you do not yet know the printer profile.

Use CMYK when the output target is known and you want composition to happen directly in printer space.

Use Lab when you need a device-independent interchange space or when source material already arrives in Lab TIFF form.

## A practical beginner workflow

1. Load or create an RGB source image.
2. Create a CMYK destination image.
3. Attach the printer ICC profile to the destination.
4. Build a transform from `:srgb` to that destination.
5. Use `bitblt()` to convert and place the source.

## Common mistakes

- Forgetting to attach a CMYK profile before building a transform.
- Treating RGB versus BGR as a colour-space difference instead of a channel-order difference.
- Rebuilding the same transform for every small copy instead of reusing it.

## Related pages

- [Home](./../)
- [BitBlt algorithms](../bitblt-algorithms/)
- [Dithering](../additional-features-guide/)
