---
title: OpenCL
description: What OpenCL is, why the GPU backend matters, and how to use it with the Graphics Library.
---

# OpenCL, the GPU backend, and this library

OpenCL is an open standard for running data-parallel work across heterogeneous hardware such as CPUs and GPUs. In practice, for this library, OpenCL is the mechanism that allows the GPU backend to accelerate the parts of the image pipeline that map well to wide parallel execution.

## What OpenCL is

OpenCL gives you a way to:

- allocate and manage device memory,
- compile and run kernels on a GPU or other compute device,
- move data between CPU-visible memory and device memory,
- coordinate work across large numbers of pixels in parallel.

You do not have to write custom kernels to benefit from it in this library. The Graphics Library already uses OpenCL internally when you choose the GPU backend.

## Why it is used here

The library is aimed at print-oriented image processing, where many operations are naturally per-pixel or per-region and can be parallelized well. That makes the GPU a good fit for workloads such as:

- large `bitblt()` operations,
- repeated colour conversion during composition,
- dithering,
- other throughput-heavy image transforms.

The main tradeoff is that GPU arithmetic is only part of the cost. Moving images between CPU memory and GPU memory can easily become the bottleneck if the workload is small or if data is transferred back and forth too often.

## When to use the GPU backend

Use the GPU backend when:

- images are large,
- the workload is repetitive,
- the application is throughput-sensitive,
- assets can remain GPU-resident for multiple operations.

Stay on the CPU backend when:

- the workload is small or irregular,
- the machine has weak or unsupported OpenCL hardware,
- most of the pipeline still needs CPU-visible image data after every step.

Starting on the CPU is still the best way to learn the API. Move to the GPU backend when you understand the data flow and can keep transfers under control.

## How to use it with this library

The library exposes the GPU backend through `jett::init(true)`.

```cpp
#include "GraphicsLibrary/jett.h"

int main(int argc, const char* argv[])
{
	jett r;
	r.init(true);

	jett_image src;
	src.loadFromFile(_T("input.tiff"), image_mode_gpu_ro);

	jett_image dst;
	dst.createImage(src.getWidth(), src.getHeight(), image_cmyk, false, image_mode_default);
	dst.set_profile_data(_T("USWebCoatedSWOP.icc"));

	jett_transform transform = r.build_transform(_T(":srgb"), dst, INTENT_PERCEPTUAL);
	r.bitblt(transform, src, 0, 0, src.getWidth(), src.getHeight(),
			 dst, 0, 0, dst.getWidth(), dst.getHeight(), 0);

	dst.saveToFile(_T("output.tiff"));
	r.destroy_transform(transform);
}
```

That is the important practical point: choosing OpenCL in this library is mostly about selecting the GPU backend and then choosing image cache policy sensibly.

## Memory caching and I/O type

In this library the practical “I/O type” decision is the image cache policy, represented by `image_mode_t`. It controls where an image prefers to live when the OpenCL backend is active and how aggressively CPU or GPU copies are discarded.

The four modes are:

- `image_mode_default`: general-purpose mode. The image can exist on both CPU and GPU and is transferred as needed.
- `image_mode_gpu_ro`: good for source images that are uploaded once and then read repeatedly by the GPU.
- `image_mode_cpu_only`: keeps the canonical copy on the CPU and discards temporary GPU copies after operations finish.
- `image_mode_gpu_copy`: keeps the CPU copy while creating a GPU-side read-only copy for repeated use.

This matters because the wrong cache policy can turn a fast GPU pipeline into a transfer-bound pipeline.

### Choosing a mode

Use `image_mode_default` when you are unsure.

Use `image_mode_gpu_ro` for source textures, overlays, or masks that are loaded once and sampled many times.

Use `image_mode_cpu_only` when an image needs to remain primarily CPU-visible and GPU work is occasional.

Use `image_mode_gpu_copy` when the CPU copy still matters, but the GPU should also be able to reuse a read-only copy efficiently.

## Direct OpenCL access

Advanced users can integrate the library into a wider OpenCL workflow through:

- `jett::cache_image(image, read_only)` to force an image into GPU memory,
- `jett::lockCLData(image, read_only)` to obtain a `cl_mem` handle,
- `jett::unlockCLData(image)` to release the lock.

Those APIs are only valid when the context was initialized with `init(true)`.

```cpp
jett r;
r.init(true);

jett_image image;
image.loadFromFile(_T("input.tiff"), image_mode_gpu_ro);

r.cache_image(image, true);
cl_mem handle = r.lockCLData(image, true);

// Pass handle to your own OpenCL code here.

r.unlockCLData(image);
```

Pair every successful `lockCLData()` call with `unlockCLData()`. Treat those calls as an advanced escape hatch, not the normal way to use the library.

## Practical guidance

- Build the pipeline on the CPU first.
- Switch to `init(true)` only after the data flow is clear.
- Keep source assets GPU-resident when they are reused.
- Avoid reading images back to the CPU between every stage.
- Expect `jett_exception` if OpenCL initialization fails on the target machine.

## Related pages

- [Home](./../)
- [BitBlt algorithms](../bitblt-algorithms/)
- [Image I/O guide](../image-io-guide/)
- [Dithering](../additional-features-guide/)