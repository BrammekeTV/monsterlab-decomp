# sir_decoder — Monster Lab (Wii) `.sir` extractor

Extracts textures, meshes and skeletons from the game's `.sir` archives
(`DATA/files/streams/*.sir` on the extracted disc).

## Setup

```
pip install numpy pillow
```

Python 3.8 or newer.

## Usage

```
python sir_decoder.py mhartopia_arena2.sir              # extract everything to mhartopia_arena2_out\
python sir_decoder.py shared_parts.sir -o D:\rip\parts  # choose the output folder
python sir_decoder.py village_arena.sir --list          # only list what's inside
python sir_decoder.py shared_parts.sir --only hum       # only items whose name contains "hum"
python sir_decoder.py some.sir --sections               # also dump the raw sections (.bin)
python sir_decoder.py some.sir --no-textures            # meshes/skeletons only
python sir_decoder.py global_binary.sir --gamedb        # print the game database as text (no numpy needed)
```

To do a whole folder (Windows PowerShell):

```
Get-ChildItem streams\*.sir | ForEach-Object { python sir_decoder.py $_.FullName -o "rip\$($_.BaseName)" }
```

## Output

```
<name>_out\
  index.json                 everything found, per section (names, sizes, formats, offsets)
  section<N>\
    textures\*.png           decoded textures (top mip level)
    meshes\*.obj             one OBJ per object, with normals and UVs; submeshes as OBJ groups
    skeletons\*.json         joints of skinned parts: inverse-bind matrix + bind position
```

- **Textures:** CMPR, RGBA8, RGB5A3, RGB565, I4, I8, IA4 and IA8. The palette formats (C4/C8/C14X2) are listed but not decoded.
- **Skinned monster parts** (names like `p_to_mbo001_geo`: `to` = torso, `hd` = head, `la`/`ra` = left/right arm, `lg` = legs) are written in their own part space at real size. The scale factor stored with each part is applied automatically. Each part sits at its attachment socket's origin, so parts load stacked at the origin rather than assembled.
- **Static meshes** (level geometry, props, effects) are written in world space, so all the level pieces of an arena line up when imported together.
- **Names:** meshes take the nearest name stored in the archive. Unnamed level pieces are called `mesh_<offset>`.

## Limitations

- Material → texture links aren't decoded, so OBJs don't reference a texture. Assign the PNGs by hand. For example, the Mhartopia boss parts all use `baronboss.png`.
- Animations (`*_mboss_*` names) aren't decoded yet. Skeletons give the bind pose only.
- `catalog.sir` is an index/lookup file, not an asset container, so nothing is extracted from it.

## Format notes (reverse-engineered)

- **Header:** big-endian, magic `PNP0`, version, section count. The section table starts at `0x40`, 24 bytes per entry: `i32 type, u32 offset, u32 size, u32, u32, u32 hash`.
- **Sections:** each section is a raw memory image the game loads at a fixed address (e.g. `0x90980000`). Pointers inside are absolute Wii addresses. The load address is recovered automatically by matching texture data pointers and mesh vertex-array pointers to where that data actually sits in the section.
- **Texture header:** `0x54` bytes, holding `u32 0x54`, `u16 w, h, w, h`, `u32 mip count` at +0x10, `u8 format` at +0x14, `u32 data size` at +0x18 and `u32 data pointer` at +0x1C. The name is at +0x35.
- **Skinned part:** the name is preceded by a float scale factor (and bounding data), then `0x7F800001` padding, then a header: `size, ?, ?, 0x60, position array offset, ?, command stream offset, ...`.
- **Meshes:** standard GX command streams. CP writes set the array bases (`0xA0`–`0xA4`), strides (`0xB0`–`0xB4`), the vertex descriptor (`0x50`/`0x60`) and the vertex formats (`0x70`+). These are followed by indexed draws (triangles, strips and fans).
- **Game database (`global_binary.sir`):** 42 tables that each store their own column types, sizes and name hashes. Layout in `docs/loader.md`; `--gamedb` prints every row.
- **Skeleton:** after the `*_geoA` name come two arrays of 4×3 matrices (rotation rows, then translation), one entry per joint. The first array holds the inverse bind matrices.
