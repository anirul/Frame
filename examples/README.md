# Examples

## 00 Cubemap

Loads `asset/json/cubemap.json` and keeps the skybox/cubemap path available in
the reduced sample set.

## 01 Refraction

![Screenshot of the refraction app.](https://github.com/anirul/Frame/raw/master/examples/raytracing.png)

The refraction sample. It loads `asset/json/raytracing.json` and is one of the
active examples kept during the renderer cleanup. Pass
`--rendering=rasterise` to render the same scene through the raster path.

## 02 Dragon

The dragon sample. It loads `asset/json/dragon.json` and is the main
reference for the acceleration-structure path across both backends. Pass
`--rendering=rasterise` to render the same scene through the raster path.

## 03 Skinned Mesh

Loads `asset/json/skinned_mesh.json`, which imports
`asset/model/cesium_man/CesiumMan.glb` as raytraced scene geometry in
`SCENE_RENDER_TIME` and plays the model's first embedded animation through the
shared raytracing pipeline.
