# sdf [![Build](https://github.com/nsmkhn/sdf/actions/workflows/build.yml/badge.svg)](https://github.com/nsmkhn/sdf/actions/workflows/build.yml)

Signed distance field renderer written from scratch in C. No dependencies beyond the C standard library — output is [PPM images](https://netpbm.sourceforge.net/doc/ppm.html) and an MP4 animation.

Companion source for the blog post: **[Signed distance fields from scratch in C](https://tokarevxvi.dev/blog/signed-distance-fields-c/)**

## What it generates

| File                     | Description                                          |
| ------------------------ | ---------------------------------------------------- |
| `circle_hard.ppm`        | Hard-threshold circle (no anti-aliasing)             |
| `circle_smooth.ppm`      | Anti-aliased circle via smoothstep                   |
| `box_smooth.ppm`         | Box SDF                                              |
| `rounded_box_smooth.ppm` | Rounded box SDF                                      |
| `shapes.ppm`             | Circle, box, and rounded box side by side            |
| `booleans.ppm`           | Union, intersection, and subtraction                 |
| `smooth_union.ppm`       | Hard union vs smooth union (smin)                    |
| `domain_distortion.ppm`  | Domain warping: rippled circle with glow             |
| `scene.ppm`              | Static frame of the space scene                      |
| `metaballs.mp4`          | Animated metaballs with smin merge (60fps, 10s loop) |

## Build

Requires a C compiler and `ffmpeg`:

```bash
# macOS: brew bundle
# Linux: sudo apt-get install ffmpeg
# Windows: choco install ffmpeg make (or download from ffmpeg.org)

make animate  # compile, generate images and animation
```

The Makefile detects the OS and uses the appropriate compiler (`cc` on macOS/Linux, `cl` on Windows).

## Clean

```bash
make clean    # removes bin/ and output/
```

## Structure

```
sdf/
├── src/main.c   # everything — SDFs, rendering, scene, animation
├── Makefile
├── Brewfile
└── output/      # generated (gitignored)
```
