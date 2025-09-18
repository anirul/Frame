# Examples

## 00 Japanese Flag

![Screenshot of the Japanese flag app.](https://github.com/anirul/Frame/raw/master/examples/japaneseflag.png)

A minimal shader demo that renders a simple analytic flag pattern. Useful for
sanity-checking your toolchain and shader hot-reload workflow.

## 01 Ray Marching

![Screenshot of the ray marching app.](https://github.com/anirul/Frame/raw/master/examples/raymarching.png)

A GLSL ray-marcher showing procedural distance fields and animated uniforms.
Great for experimenting with screen-space shading without scene assets.

## 02 Raytracing

![Screenshot of the ray tracing app.](https://github.com/anirul/Frame/raw/master/examples/raytracing.png)

The primary sample for Frame. It loads `asset/json/raytracing.json`, builds the
scene acceleration data, and shades the results with physically-based textures.
Use this as the reference when extending the renderer or adding compute stages.
