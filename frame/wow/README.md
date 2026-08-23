# Retail WoW M2 proof of concept

This module directly reads the current chunked Retail M2 layout. It does not
use Assimp, WoW Model Viewer, CASCLib, a converter, or an intermediate GLB.

The implemented slice is intentionally small:

- `MD21` model payload and its `MD20`-layout header
- 48-byte Retail M2 vertices (position, normal, primary UV and bone data)
- `SFID` skin and `TXID` texture FileDataID discovery
- `SKIN` vertex/triangle lookup reconstruction
- static bind-pose output for Frame's raster and raytrace buffers

## Loose-file layout

Frame does not open the running game or its CASC installation. Put an extracted
M2 below `asset/model/` and put its first skin profile beside it. Given
`character.m2`, the loader checks these names in order:

1. `character.skin`
2. `character00.skin`
3. `<first SFID FileDataID>.skin`
4. `<first SFID FileDataID>`

This supports both human-named exports and FileDataID-named Retail exports.

Reference the model from a normal Frame scene node:

```json
{
    "name": "RetailCharacter",
    "file_name": "wow/character.m2",
    "acceleration_structure_enum": "BVH_ACCELERATION"
}
```

The OpenGL and Vulkan scene loaders both create point, normal, UV, index,
raytrace-triangle, and (when requested) BVH buffers directly from the WoW data.

## Current limits

This first proof of concept displays unanimated geometry with the scene's
existing material. BLP decoding, M2 render batches, character customization,
attachments, and skeletal animation are separate follow-up slices. The parser
retains texture FileDataIDs so BLP/material support can be added without
changing the geometry API.
