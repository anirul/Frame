# Examples

## 00 Cubemap

Loads `asset/json/cubemap.json` and keeps the skybox/cubemap path available in
the reduced sample set.

## 01 Raytracing

![Screenshot of the ray tracing app.](https://github.com/anirul/Frame/raw/master/examples/raytracing.png)

The simple raytracing sample. It loads `asset/json/raytracing.json` and is one
of the active examples kept during the renderer cleanup.

## 02 Dragon

The dragon sample. It loads `asset/json/dragon.json` and is the main
reference for the acceleration-structure path across both backends.

## 03 Skinned Mesh

Loads `asset/model/fox/Fox.glb` through the OpenGL glTF import path and renders
it with the `scene_simple` raster pipeline. If the glTF contains skinning data,
the mesh currently renders in bind pose while animation playback is still being
implemented.
