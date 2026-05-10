---
title: Polygon Algorithms
---

# How polygon drawing and anti-aliasing work

This page explains the drawing model behind `jett::polygon()` from the point of view of someone still building graphics intuition. The line pipeline is covered separately in [Line drawing](../line-drawing/).

## The mental model

Think of the destination image as a grid of square buckets. A drawing algorithm has two jobs:

1. Decide which buckets are covered by the shape.
2. Decide how much of each bucket is covered.

- `polygon_fast` and `line_fast` choose speed. A pixel is either in or out.
- `polygon_best` and `line_best` choose smoother edges by blending based on coverage.

## How polygon filling works

The CPU polygon filler in `CPUPolygon.cpp` uses a classic scanline algorithm:

1. Intersect the row with every polygon edge.
2. Build a list of X positions where the row enters or leaves the polygon.
3. Sort those intersections.
4. Fill between each pair.

That approach is easy to clip, easy to parallelise by row, and works well for solid fills.

## How polygon anti-aliasing works

In `polygon_best` mode the library uses oversampling. Instead of testing coverage once per output pixel, it evaluates the polygon on a finer sub-pixel grid and blends each pixel according to the fraction of sub-samples that are covered.

The point is not that the library blurs the whole shape. It computes more accurate edge coverage.

<div class="doc-gallery">
	<figure class="doc-figure">
		<img src="{{ '/images/polygon-best.png' | relative_url }}" alt="A filled magenta star with anti-aliased edges.">
		<figcaption><code>polygon_best</code> keeps the star edges visually smoother.</figcaption>
	</figure>
	<figure class="doc-figure">
		<img src="{{ '/images/polygon-fast.png' | relative_url }}" alt="A filled magenta star rendered with the fast polygon mode.">
		<figcaption><code>polygon_fast</code> is quicker, but the edge treatment is harsher.</figcaption>
	</figure>
</div>

## Rectangle drawing

`jett::rectangle()` is effectively a convenience wrapper around polygon filling. It is worth calling out separately because it is often the first primitive people use when testing a new image surface or colour space.

If you understand the polygon fill model, you already understand what rectangle drawing is doing under the hood.

## When to choose fast versus best

Use the fast flags when throughput matters more than screen appearance, or when the result will be dithered later for print.

Use the best flags when the output will be inspected on screen or when the artwork contains diagonals and fine shapes that benefit from softer edges.

## Practical examples

- A UI-like vector badge usually wants `polygon_best`.
- A large filled mask for later printing often works well with `polygon_fast`.

## Related pages

- [Home](./../)
- [BitBlt algorithms](../bitblt-algorithms/)
- [Line drawing](../line-drawing/)

