# PC port: big-endian data loader

Roadmap step 3. The game's data files are PowerPC memory images: big-endian, with 32-bit absolute
pointers. The PC build is little-endian Win32. This note says how the original loads the files,
how the port converts them, and what is implemented.

Code: `include/port/sir_loader.h`, `src/port/sir_loader.cpp`. Test: `tests/sir_loader_test.cpp`,
`python tools/loader_check.py`. Reference decoder: `python tools/sir_decoder.py <file> --gamedb`.

## Status

| | |
|---|---|
| Container (`PNP0` header, section table) | parsed |
| `global_binary.sir` header block (4 KB) | converted; 8 slots typed from code, the rest from the data (listed in the source) |
| `global_binary.sir` game database (42 tables, 2,591 rows) | converted from the schema stored in the file |
| Rest of the `global_binary.sir` image (about 88%: textures, models, string table, UI, fonts) | not converted yet |
| Hook into the game's `SirLoad` | not wired (the port has no file layer yet) |

Check against the real file: all 3,823 compared lines (every header slot, table descriptor and row,
plus parts, actions and tuning values read through the game's own structs and offsets) match the
Python reference.

## How the original loads a file

1. `SirLoad(loader, name, dst, size, type)` (0x801E6138) hashes the name and looks it up, with the
   section type, in the file table at `loader + 0x7B4` (0x18-byte entries: type, disc offset, size,
   name hash, compression fields; presumably filled from `catalog.sir`). It then queues a disc read of the
   section straight into `dst`. `SirFinishLoad` (0x801E6C44) runs the queue until the read is done.
2. There is **no fix-up step**. A section is a memory image built for one fixed address, and the
   caller passes exactly that address as `dst`. `GameStart` reads section 0 of `global_binary.sir`
   (type -1, 4 KB) into `sSirHeader` and section 1 (type 0) to `0x90400000`.
3. Pointers in the image are absolute Wii addresses of three kinds:
   - into the image itself (`0x904xxxxx`);
   - into `main.dol`: vtables and callbacks of objects stored in the file (about 2,500 words in
     the image look like such addresses, 47 distinct targets). The game database alone has 87:
     its own vtable-like pointer `0x8049F444` and two array callbacks, `0x80072D74` and
     `0x80072D98`, repeated in every table. So the files only work with the exact executable
     they were built for;
   - NULL.
4. Memory the build tool never wrote holds the fill word `0x7F800001` (struct padding, unused
   array slots). It is not data.

## Converting on PC

The decompiled code reads fields at their original offsets (`*(s32*)(row + 4)`,
`*(u8**)(gGameDb->data + 0x5B78)`). So the loader converts **in place**: same offsets and sizes,
host byte order, and each 4-byte pointer slot holds a host pointer. No struct is re-laid-out.

### Byte order: by field type

A field is swapped according to its type, never by a blanket pass over words:

| Type | Conversion |
|---|---|
| `u8`, `s8`, flags, strings, byte arrays, GX display lists and texture data | none |
| `u16`, `s16` | 2-byte swap |
| `u32`, `s32`, hash, `f32` | 4-byte swap |
| pointer | 4-byte swap, then relocation (below) |

A blanket swap is wrong in practice. A game database row such as `{s32 id; u8 value;}` is stored
as `00 00 00 05 | 64 80 00 01`: the byte `0x64` followed by three fill bytes. Swapping the second
word moves `0x64` from offset 4 to offset 7. The self-test covers this case.

Anything no schema names stays as it was read from disc.

### Where the types come from

1. **The file itself (game database).** Header slot `0xDC8` points at the database object; its
   word `+0xC` (`GameDb::data`) points at 42 table descriptors of `0x428` bytes each. The code's
   table offsets are descriptor fields: `data + 0x5B78` is descriptor 22's row pointer and
   `+0x5B7C` its row count. Each descriptor describes its rows:

   | Offset | Field |
   |---|---|
   | `0x000` | two `main.dol` function pointers (array callbacks) |
   | `0x008`, `0x00C` | row pointer, row count |
   | `0x010` | 1 |
   | `0x014` | hash of the layout |
   | `0x018`, `0x01C` | row size, row count again |
   | `0x020` | `u32 type[64]` |
   | `0x120` | `u32 size[64]` |
   | `0x220` | `u32 nameHash[64]`: `StringHash` of the column name |
   | `0x320` | `u32 refHash[64]`: `StringHash` of the table an id column refers to (`PART`, `ACTION`, `ENHANCEMENT`, `DEFECT`, `INGREDIENT`, `RECIPE`, `QUEST`, ...), or -1 |
   | `0x420`, `0x424` | field count, `u8` present flag |

   Type codes (the names are ours; the file gives only the code and the size):
   3 = `s32`, 4 = `f32`, 5 = `u32` hash, 6 = `s16`, 7 = `u8` flag, 8 = `u8`, 11 = 4 bytes (one
   column, -1 in every row). Fields are packed in order, each aligned to its own size, and a row
   is padded to 4 bytes. That rule reproduces the stored row size of all 41 non-empty tables.

   The loader reads this schema and converts rows by it. A mod can therefore change rows, add
   rows, or change a table's columns without any change to the loader. The same schema gives the
   mod tools a full column list per table for free.

2. **Hand-written schemas, built from what the decompiled code reads.** Where a file does not
   describe itself, a struct gets a table of `SirField {offset, type, count}` entries in
   `sir_loader.cpp` and is converted with `SirConvertRecord`. Three exist so far: the database
   object, the table descriptor, and the 4 KB header block. Each entry of the header schema is
   marked `code` (a decompiled function reads the slot with that type, e.g. `0x824` texture
   pointer, `0x820` flag byte) or `data` (typed only from the values in the file; to confirm when
   the reading code is decompiled). Words not listed are 0 or -1 in the shipped file.

   New schemas should come from the same evidence: the access width and type in the decompiled
   code (`*(s16*)(row + 0x1C)`, `*(f32*)...`, a pointer dereference). Once a layout is solid it
   becomes a named struct in `include/`, and the schema is written next to it.

### Pointer slots in the Win32 build

A pointer slot is 4 bytes in the data and `sizeof(void*)` is 4 in the Win32 build, so a host
pointer fits in the slot and `*(u8**)(p + off)` in the decompiled code just works. The loader
keeps a list of ranges, `SirReloc`: Wii base address, size, host buffer. A pointer field is
converted like this:

| Wii value | Slot after conversion |
|---|---|
| 0 | NULL |
| inside a mapped range (one past the end included) | `host + (value - wiiBase)` |
| `main.dol` address (`0x80000000`–`0x817FFFFF`) | the resolver's answer: the PC build's object for that address |
| `main.dol` address the resolver does not know, or anything else | the Wii address, in host byte order |

The last row is deliberate. The port never maps the Wii's address ranges, so such a pointer is
easy to recognise in a debugger and faults when used. The load report counts them
(`ptrDolKept`, `ptrUnknown`).

The **resolver** is a callback, `void* (*)(u32 wiiAddr, void*)`. The port will fill it from a
generated table of `{original address, &symbol}` pairs for the vtables and callbacks that data
files reference. `symbols.txt` already has the addresses; the list of needed ones is exactly
what the load report prints. None are registered yet.

**Why relocate instead of reserving `0x90400000`.** A large-address-aware Win32 process could map
the image at its Wii address and skip relocation of internal pointers. We do not rely on it:
`main.dol` pointers need translating anyway, every pointer field must be known to be swapped at
all, a fixed address can be taken by a DLL or the heap, and it would tie the data format to
32-bit for good. Relocation costs nothing once the fields are typed. (If the platform layer does
map an image at its original address, the same code runs with a delta of zero.)

**64-bit later.** In-place conversion needs 4-byte pointers. A 64-bit build needs named structs
with real pointer members; the loader then builds those from the same schemas instead of
patching slots. The game database is the easy case, because its rows hold no pointers at all:
only the descriptor's row pointer has to widen.

### Safety

Files will come from mods, so the loader does not trust them. For the game database it first
validates everything (descriptor signature, field count, known type codes with matching sizes,
computed row size equal to the stored one, rows inside the image, no two tables or descriptors
sharing bytes) and only then changes the first byte. A file it does not understand is rejected
whole, with the failing table index. Hand-written schemas are bounds-checked against the record
size. Conversion happens once per load; nothing in the data marks it as converted, so the caller
owns that.

### Where it plugs into the game

`SirLoad` and `SirFinishLoad` are decompiled game code and stay as they are. The port replaces
the disc read beneath them. When a section's read completes, the file layer calls the converter
for that file and section type. For `global_binary.sir` that is `GlobalBinaryConvert(reloc, hdr,
image, size, &failTable)` after both sections are in memory (the header's pointers need the image
mapped first). `MEM2_GLOBAL_BINARY` becomes the host buffer. `GlobalBinaryLoadFile` does the same
from a path with stdio, for tools and tests.

## Test

```
python tools/loader_check.py
```

1. Builds `tests/sir_loader_test.cpp` with MSVC (Win32).
2. `--selftest`: a small big-endian database built in memory, including the byte-plus-fill row
   above, a resolved and an unresolved `main.dol` pointer, and seven malformed files that must be
   refused with the image left untouched. CI runs this step (no game data needed).
3. With the real file in `orig/` (never committed): the C++ loader converts it and prints every
   header slot, descriptor and row, then parts, actions and tuning values through `PartDef`,
   `PartAction`, `PartActionType` and the `data + 0x99D0` tuning rows, as the game reads them.
   The output must equal `tools/sir_decoder.py --gamedb`, which decodes the big-endian bytes
   directly in Python.

Result on PAL `RM7P4F`: 18 header pointers, 13 header words, 3 header bytes, 42 tables,
382 columns, 2,591 rows, 186 parts, 372 actions and 215 tuning values, all equal. The load
report shows 60 image pointers relocated, 87 `main.dol` pointers kept, none unknown.

## Found on the way

- `PartDef` (`include/game/part.h`) ends with five `s16` at `0x40`–`0x49` and three `u8` at
  `0x4A`–`0x4C` according to the file's schema, not four `u16` and two `u8[4]`.
- The column-name hashes are not in `tools/hashes.py`'s dictionary (the names are not in the
  disc's strings). The reference names (`PART`, `ACTION`, ...) are. Guessing column names from
  the reference names would label most tables.

## Next

In the order the boot path needs them:

1. **`catalog.sir`**: the file table `SirLoad` searches. Nothing loads without it.
2. **The rest of `global_binary.sir`**: the string table (header slot `0xDA8`, 0x10B2 entries),
   the two objects the boot code hands to the engine (`0x824`, `0x828`: both start with a
   `0x54`-byte GX texture header; pixel data stays big-endian for the GX layer), the screen-set list
   (`0xD98`), and the objects at `0xDB0`–`0xDC4`. Each needs a hand-written schema.
3. **A `main.dol` pointer table** for the resolver, generated from `symbols.txt`.
4. **`shared_ui.sir`** screens (title screen), then **`shared_texture.sir`**.
5. **Save files**: written from memory by the Wii build, so big-endian too.
6. **`shared_parts.sir`** and the level and arena files: models, skeletons, textures.
