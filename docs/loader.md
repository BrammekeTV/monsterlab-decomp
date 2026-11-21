# PC port: big-endian data loader

Roadmap step 3. The game's data files are PowerPC memory images: big-endian, with 32-bit absolute
pointers. The PC build is little-endian Win32. This note says how the original loads the files,
how the port converts them, and what is implemented.

Code: `include/port/sir_loader.h`, `src/port/sir_loader.cpp`. Test: `tests/sir_loader_test.cpp`,
`python tools/loader_check.py`. Reference decoder: `python tools/sir_decoder.py <file> --gamedb`
(game database), `--image` (the other objects of `global_binary.sir`), `--catalog`.

## Status

| | |
|---|---|
| Container (`PNP0` header, section table) | parsed |
| `catalog.sir` (file table: 6,394 section entries, 59 file records) | converted |
| `global_binary.sir` header block (4 KB) | converted; 14 slots typed from code, the rest from the data (listed in the source) |
| `global_binary.sir` game database (42 tables, 2,591 rows) | converted from the schema stored in the file |
| `global_binary.sir` input table (slot `0xDC4`: 98 sequences, 101 steps) | converted; typed from code, except the last 0x38 bytes of the table (from the data) |
| `global_binary.sir` textures (slot `0xD94`: list of 44, including the two at `0x824` and `0x828`) | headers converted; pixel data and palettes stay in GX layout |
| `global_binary.sir` string table (slot `0xDA8`: 4,274 strings) | converted |
| `global_binary.sir` action table (slot `0x844`) and its 956 records | converted: a record is a program of 32-bit words; the 42 sound objects its calls point at are not |
| `global_binary.sir` screen list and HUD (slots `0xD98`, `0xD9C`: 68 state machines, 68 definitions, 759 states, 1,345 transitions) | converted; every field typed from code, fields without a reader left as they are |
| `global_binary.sir` UI root, fonts and the objects of slots `0xDB4`-`0xDBC` | the fields the code reads are converted; the glyph data of the three fonts is not |
| `global_binary.sir` font glyph data, sound objects named by action records, `0xDC0`, `0x10`, `0x418`, `0x830`, `0x83C` | not converted: see "Not converted yet" |
| Hook into the game's `SirLoad` | not wired (the port has no file layer yet) |

Of the 1,096,124 bytes of the image, 31.0% now belongs to objects converted by a schema (the game
database is 11.5%, the screen state machines 11.2%, the action records 4.2%), 57.0% is data that
needs no conversion and is reached from a converted object (texture pixels and palettes, string
bytes), and 4.3% is fill words. About 7.7% (84 KB) is not typed yet.

Check against the real files: all 13,834 compared lines of `global_binary.sir` (every header slot,
table descriptor and row, parts, actions and tuning values, every sequence, step, texture
header, string, action pointer and action record, and every state machine, definition, state,
event, node and transition, read through the game's own structs and offsets) and all
6,457 lines of `catalog.sir` match the Python reference.

## How the original loads a file

1. At start-up `fn_801E54E4` reads `catalog.sir` into a buffer and copies two arrays out of it
   into the record at `loader + 0x7B4`: the section entries (count at buffer `+0x08`, 0x18 bytes
   each, from `+0x40`) and the file records (count at `+0x0C`, 0x40 bytes each, right after).
   `SirLoad(loader, name, dst, size, type)` (0x801E6138) hashes the name, finds the entry with
   that name hash and section type, finds the file record with the same hash (its index selects
   the open file handle), and queues a disc read of the section straight into `dst`.
   `SirFinishLoad` (0x801E6C44) runs the queue until the read is done.
2. There is **no fix-up step**. A section is a memory image built for one fixed address, and the
   caller passes exactly that address as `dst`. `GameStart` reads section 0 of `global_binary.sir`
   (type -1, 4 KB) into `sSirHeader` and section 1 (type 0) to `0x90400000`.
3. Pointers in the image are absolute Wii addresses into the image itself (`0x904xxxxx`), or
   NULL.
4. The image also holds words that look like `main.dol` addresses (about 2,500, 47 distinct
   values). The game database has 87: `0x8049F444` in the database object, and `0x80072D74` and
   `0x80072D98` at the start of every table descriptor. **These are not usable pointers.** In
   the retail PAL executable `0x80072D74` and `0x80072D98` are branch instructions in the middle
   of `CreateEntityByType`, not function entries, so the file was baked against another build
   and the retail game cannot be calling through them. The others fall in `.bss`, which is not
   split into symbols yet, so they cannot be checked the same way. The loader types them as
   plain `u32` words (`buildAddr`) until some decompiled code is shown to use one.
5. Memory the build tool never wrote holds the fill word `0x7F800001` (struct padding, unused
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
   | `0x000` | two build-time addresses (see above), the same in every table |
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
   `sir_loader.cpp` and is converted with `SirConvertRecord`: the database object, the table
   descriptor, the 4 KB header block, and the image objects described under "The other objects
   of `global_binary.sir`". Each entry is marked `code` (a decompiled function reads the field
   with that type, e.g. `0x824` texture pointer, `0x820` flag byte) or `data` (typed only from
   the values in the file; to confirm when the reading code is decompiled). Header words not
   listed are 0 or -1 in the shipped file.

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

The **resolver** is a callback, `void* (*)(u32 wiiAddr, void*)`, for real `main.dol` pointers in
data. Nothing converted so far has one (see "How the original loads a file", point 4), so no
table is generated from `symbols.txt` yet: there is nothing to put in it. If a later file has
pointer fields that do land on symbol starts, the load report (`ptrDolKept`) lists them and the
table can be generated from that list.

**Why relocate instead of reserving `0x90400000`.** A large-address-aware Win32 process could map
the image at its Wii address and skip relocation of internal pointers. We do not rely on it:
every pointer field must be known to be swapped at
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
size. The hand-typed image objects are checked the same way: every count, type code and pointer
is read from the big-endian bytes first and each object is recorded as a region; a region outside
the image, a texture list whose links disagree, or two regions that share bytes rejects the file
before anything changes. The regions are kept in a list that is sorted once, so the overlap test
stays cheap with the 3,500 objects of the screen state machines; an object that several others
point to (a definition used by two machines, a transition used by two events) is one region. Conversion happens once per load; nothing in the data marks it as
converted, so the caller owns that.

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
   `PartAction`, `PartActionType` and the `data + 0x99D0` tuning rows, as the game reads them,
   then the image objects through `InputSequence`, `SirTexture`, `SirStringTable` and
   `SirActionTable`, then the UI objects and the screen state machines at the offsets the game
   code uses. Words the loader must leave alone (no reader in the code) are printed as raw bytes,
   so a conversion that touched them would show. The output must equal `tools/sir_decoder.py --gamedb --image`, which decodes
   the big-endian bytes directly in Python. Texture pixels and string bytes are compared by a
   checksum, so a conversion that touched them would show.

The self-test also builds an input table, two textures, a string table, an action table, a UI
root with a font, and two state machines that share a definition with one node of each type, and
two action records, and 43 broken variants of them (bad counts, unknown step class or node type,
list loops, pointers off the image, overlapping objects, one transition used as two kinds, an
instruction of length 0, a jump into an operand, a record without an end) that must be refused
untouched.

Result on PAL `RM7P4F`:

- `global_binary.sir`: 18 header pointers, 13 header words, 3 header bytes, 42 tables,
  382 columns, 2,591 rows, 186 parts, 372 actions and 215 tuning values; 98 input sequences,
  101 steps, 44 textures, 4,274 strings, 956 action pointers and 956 action records (11,526
  words); the UI root, 3 fonts, 4 font
  objects, 68 state machines with their value tables, 68 definitions, 759 states, 1,104 events,
  59 nodes, 23 shared events and 1,345 transitions, all equal. The load report shows 9,425 image
  pointers relocated and none kept or unknown.
- `catalog.sir`: 6,394 entries and 59 file records, all equal. All 59 file-name hashes equal
  `StringHash` of the stored name, and the two lookups `GameStart` makes for
  `global_binary.sir` find sector 1 / 4,096 bytes and sector 3 / 1,096,124 bytes.
- Python only: the catalog's entries agree with the section table inside each of the 59 files.

## catalog.sir

Container header, then the two arrays `fn_801E54E4` copies:

| Offset | Field |
|---|---|
| `0x00` | `PNP0` |
| `0x04`, `0x08`, `0x0C`, `0x10` | version 3, entry count, file count, hash |
| `0x40` | entries, 0x18 bytes: `s32 type, u32 offset, u32 size, u32 nameHash, u32 packedSize, u32 hash` |
| after the entries | file records, 0x40 bytes: `u32 nameHash, u32 unk, char name[0x38]` |

An entry is the section-table entry of the `.sir` file itself with two changes: the file-name
hash at `+0x0C`, and at `+0x10` the size on disc when the section is compressed (the `...C.sir`
files), else 0. `SirCatalogConvert` swaps the header words, six words per entry and two per
file record, and leaves the names alone. `SirCatalogFind` and `SirCatalogFileIndex` do the two
searches `SirLoad` does.

On PC the port's file layer converts the buffer once, between the read and the copy in
`fn_801E54E4`. File names in the records are lower-case (`castle_arm_wrestlec.sir` for
`castle_arm_wrestleC.sir`), which Windows resolves; a case-sensitive file system needs a lookup.

## The other objects of `global_binary.sir`

Structs and constants are in `include/port/sir_loader.h`; the schemas, with the function each
field was typed from, in `src/port/sir_loader.cpp`.

### Input table (header slot `0xDC4`)

`GameStart` passes it to `fn_800214D4`; `RemoteInputGet(table, id)` returns one input.

| Offset | Field |
|---|---|
| `0x000` | 8 sequences of 0x48 bytes |
| `0x240` | up to 100 more sequences |
| `0x1E60` | `s32 index[34]`: sequence number (in the array at `0x240`) for each input id |
| `0x1EE8` | `s32` sequences in use at `0x240` (90) |
| `0x1EEC`-`0x1F23` | six pointers to the first six sequences, then `0, -, 1.0, 1, 0, 320.0, 240.0, 0`: typed from the data only |

A sequence (`InputSequence`) is a step count, the current step, two values, ten step pointers,
three times and a flag: `RemoteInputUpdate` reads all of it. A step is one of ten classes,
chosen by the type word at `+0x48`; the names are the classes' RTTI names in `main.dol`:

| Type | Class | Bytes in the file | State behind the common 0x50 bytes |
|---|---|---|---|
| 0 | `CAbstractInputButtonHeld` | 0x50 | none |
| 1 | `CAbstractInputButtonJustReleased` | 0x60 | `f32` previous value |
| 2 | `CAbstractInputButtonJustPressed` | 0x60 | `f32` previous value |
| 3 | `CAbstractInputTwoButtonToggle` | 0x50 | none in the file (see "Found on the way") |
| 4 | `CAbstractInputAnyButtonsHeld` | 0x50 | none |
| 5 | `CAbstractInputAnyButtonsReleased` | 0x60 | `f32` previous value per source (4) |
| 6 | `CAbstractInputAnyButtonsJustPressed` | 0x60 | `f32` previous value per source (4) |
| 7 | `CAbstractInputRotateCW` | 0x60 | `s32` direction (-1), `f32` timer, time, steps |
| 8 | `CAbstractInputRotateCCW` | 0x60 | same |
| 9 | `CAbstractInputToggle` | 0x60 | `f32` previous value |

The common part is `f32 raw, f32 value, s32 sourceCount`, up to four sources of 12 bytes
(`u32 id, f32 value, u32 kind`; `fn_8001FCA8` switches on the kind, 0-0x1C), `s32` at `0x3C`,
an `f32` range at `0x40`, the type, and a vtable slot at `0x4C`. Only `sourceCount` sources are
converted; the unused ones hold fill words. The vtable slot holds a build-time address
(`0x8048C4B0`, ...) and is typed as a plain word: `fn_80020334`, called from `fn_800214D4` at
boot, stores the real vtable for the step's type there before anything uses it. On PC that boot
code does the same with the port's vtables, so no resolver entry is needed.

### Textures (header slot `0xD94`, and `0x824`, `0x828`)

Slot `0xD94` points at the start of the image: a word (`0x42`), then a list `{head, tail,
count}` of all 44 textures in the file, linked through `next` / `prev` / `list` pointers at
`+0x28` of each texture. This list is typed from the data only; the loader walks it to find the
textures and checks every link. Slots `0x824` (`Icon-HomeButtonMenu_is_not_available`, kept by
`fn_801FC038`) and `0x828` (`Generic_box`, kept by `fn_801D5D0C`) point at the first two. Both
are textures: `Generic_box` is a 16x16 palette texture, not a model.

Texture header (`SirTexture`): `u32` 0x54, `u16` width and height twice (`fn_801FCA38` reads the
first pair, `fn_80195A58` writes the second to the GX image register), twelve bytes (`0x0C`-`0x17`;
mip count, GX format, palette format and others read by `fn_80195A58`; four have no known reader),
`u32` data size, pointers to the pixel data and the palette, a small count, the three list
pointers, a byte and the name. Pixel data and palette entries are **not** converted: they are in
GX layout, for the port's GX layer to decode. `fn_80195A58` turns the data pointer into a
physical address for a GX register (`>> 5`); on PC the GX layer has to take the host pointer
there instead.

### String table (header slot `0xDA8`)

`{const char** strings, ptr, u32 build-time address, u32 count, ptr text}`. The game's string
lookups (`LookupString` in `src/game/combat_hud.cpp` and its inlined copies) read `strings[id]`
for `id < count`. The loader converts the object and the 4,274 pointers; the text
is single-byte and stays as it is. The second pointer leads to zeroed words behind the array.

### Action table (header slot `0x844`)

`{u32 count, ptr records}`, read by `TriggerDoAction` for zone id 0x1FF: `records[index]` is
passed on as the address of an action record. Table, the 956 pointers and the records are
converted.

### Action records

A record is a program for the interpreter `fn_801E1BD4` (0x801E1BD4), which `TriggerDoAction`
starts for an action word of type 2 (`0x17FC0002`: type 2, zone 0x1FF, record 2). It is a run of
32-bit words. An instruction is one word and its operands:

| Bits | Field |
|---|---|
| 0-7 | opcode |
| 8-15 | length of the instruction in words, with its operands |
| 16-27 | script function index (opcode 22) or jump distance in words from this instruction (opcodes 4-17; `fn_801E239C`) |

| Opcode | What the interpreter does | Operands |
|---|---|---|
| 1 | header: `fn_801E2368` runs a record only if its first word has this opcode | - |
| 0, 2, 3, 21 | nothing | - |
| 4 | jump | - |
| 5 | jump if the result register is 0 | - |
| 6-11 | pop two ints, jump if `==`, `!=`, `>`, `<`, `>=`, `<=` | - |
| 12-17 | the same for floats | - |
| 18, 19 | push an int, a float | the value |
| 20 | convert the top of the stack from int to float | - |
| 22 | call a script function (`ScriptDispatchEngine`); the result goes to the result register | its arguments, one word each |
| 23 | run another action word (`TriggerDoAction`) | the action word |
| 24-27 | push the result register, pop into it (as int; 26 and 27 as float) | - |
| 0xFF | end | - |
| other, above 27 | skipped as one word | - |

Every operand is a 32-bit value, so the loader converts a whole record as words. Before that it
walks the instructions to the end word and refuses a record with an instruction of length 0, a
push or action without its operand, a call with more than eight arguments (the interpreter's
buffer), or a jump that does not land on an instruction of the same record. A record that does
not start with the header opcode is never run; only its first word is converted. In the shipped
file all 956 records start with `0x....0101`, hold 3,236 calls of 526 different functions, 662
jumps, and no opcode 19 or above 25.

**What a call's arguments are** depends on the function's signature (`gScriptSigs`, filled by
`ScriptSigDefine`; argument type codes per function in `src/game/script_signatures.cpp`), and the
function is known only by its index in `gScriptFuncs`. That table is filled in registration order
after `global_binary.sir` is loaded (it is empty when the file is converted and has 2,050 entries
once the text table is read), so the loader cannot look a function up. Checked once against the
tables of the running PC build:

- every one of the 3,236 calls has exactly as many operands as its signature has arguments;
- no call uses a signature with an argument of type `0xA` (`Print`, and signatures `0x47`, `0x4A`,
  `0x53`, `0x54`): those hand the function a pointer to what follows the instruction, which would
  be bytes, not words;
- arguments of type `0xB` and `0xD` are pointers into the image, and no other type ever holds one:
  `PlaySoundEffect` (2 calls) and `RegisterExperimentSound` (1) with type `0xB`,
  `LT_SetTutorialText` (39) with type `0xD`. The loader converts these 42 operands as words. That
  is right in the PC build, where the image sits at its original address `0x90400000`; a loader
  that places the image elsewhere would have to relocate them, and needs the function table for
  that.

The 42 objects those operands point at lie between the records (`0xDF2E0`, `0xE1AD0`,
`0xECE30`, ...). The three looked at have the same shape: a pointer to a second part 0x30 bytes
further on, a flag word, floats (1.0, -3.0), a hash, and in the second part of one a name
(`Generic_Timer_Countdown_Mo_01`). They are **not** converted: the functions that read them are
not decompiled far enough to type the fields.

### Screen state machines (header slots `0xD98`, `0xD9C`)

`UiInit` hands slot `0xD98` (the screen list) to `fn_801BC574` and slot `0xD9C` (the HUD) to
`HudInit` / `HudReset`; `HudUpdateScreens` (0x801BB5BC) steps a machine every frame. The HUD holds
68 machines (pointers from `+0x28`, count at `+0x428`); the first one is the screen list itself.
Every machine in the file has its own definition, but the format allows sharing one.

| Object | Bytes | Fields (all typed from code) |
|---|---|---|
| machine | 0x70 | definition, `s32` current / previous / pending state (-1), owner, six words (event arguments, flags at `+0x28`), ten floats (timer at `+0x4C`), value table at `+0x54` |
| value table | 4 + 4n | `u32 count, u32 value[count]`: what an action word of type 3 is looked up in (`fn_801BBE40`) |
| definition | 0x80 | states and count, node pointers and count (`+0x10`, `+0x1C`), shared events and count (`+0x20`, `+0x2C`), key (`+0x30`, what `fn_8001B044` searches the HUD for), the start node at `+0x34` with its transition at `+0x40`, a byte at `+0x44` |
| state | 0x30 | vtable slot, key, type (1), three action words (enter, update, exit), `f32` time-out, flags, events and count |
| event | 0xC | `u32 id, u32 condition, transition*`; a definition's shared event is `u32 id, transition*, u32 condition` |
| node | 0x30 | vtable slot, key, type, then by type (below) |
| transition | 0x20 | vtable slot, target (state or node), condition word, action word, two words |
| shared event's transition | 0x30 | the same, then flags (`+0x18`), a list of state keys (`+0x1C`) and its count (`+0x20`): the states it applies to, or with bit 0 set those it does not (`fn_801BC4C4`) |

Node types, from the vtables `fn_801BC5F0` installs:

| Type | Vtable | Fields behind the common 0xC bytes |
|---|---|---|
| 0 | `0x80383FC4` | none |
| 1 | `0x80383F90` | a state: lives in the state array, never in the node array |
| 2 | `0x80383F58` | transition (also the class of a definition's start node) |
| 3 | `0x80383F20` | action word, transition, two words copied into the machine (`fn_801BBF24`) |
| 4 | `0x80383EE8` | action word, pointer to transition pointers and their count, result, last choice: the action's result picks the transition (`fn_801BC20C`, `fn_801BC2B8`) |

The file uses types 1 and 4 only (759 states, 59 choice nodes); the loader also converts 0, 2
and 3, typed from the same code, and refuses anything else.

An action or condition word has a type in bits 27-30 (0 = none) and a value in bits 0-17. Type 3
takes the value from the machine's value table; the result is handed to `TriggerDoAction`. Those
results are indices into the action table of slot `0x844`, whose records are **not** converted
yet, so a screen that runs an action still reads big-endian script words.

The vtable slots hold build-time addresses (`0x804EE9D0`, ...). For states, nodes and the start
node the boot code stores the real vtable; nothing stores one in a transition and no code calls
through it. All are typed as plain words.

Left as they are, because no decompiled function reads them: machine `+0x58`-`+0x6F`; definition
`+0x08`, `+0x14`, `+0x18`, `+0x24`, `+0x28` (sizes next to the three arrays) and everything behind
`+0x44`; state `+0x24`, `+0x28`; transition `+0x18`, `+0x1C`.

### UI root, fonts and the objects of slots `0xDB4`-`0xDBC`

Only fields with a reader in the code are converted.

| Object | Converted | Left (no reader) |
|---|---|---|
| UI root (`0xDB0`, `gScreenSets`, 0x98 bytes) | three font pointers at `+0x14` (`ScreenSetsInit`, `fn_8018C9DC`), 19 floats at `+0x20` (a 3x4 matrix and seven more, `fn_8018CE64`), the run-time words `+0x6C`-`+0x90`, the pointer at `+0x10` (holds the fill word) | `+0x00` (1), `+0x04`-`+0x0C` (pointers to the objects of `0xDB8`, `0xDB4`, `0xDBC`) |
| font (0xD0 bytes): `converted_rf`, `MSA_Title_rf`, `strongman_fo...` | the font object pointer at `+0xCC` | `+0x00` (1); the name at `+0x04` is text |
| font object, kind 1 (0x128 bytes, right behind the font) | vtable slot, kind, font data pointer (`+0x08`), page texture pointers from `+0x24`, page count at `+0x124` | - |
| font object, kind 0 (0xC bytes; none in the file) | vtable slot, kind, pointer at `+0x08` | - |
| `0xDB4` (`lbl_805FB660`, 0xC bytes) | two font object pointers (`fn_80196D0C`, `fn_80196E7C`) and the font object they name | `+0x00` (1) |
| `0xDB8` (`lbl_805FB658`, 0x408 bytes) | `+0x04` (`UiInit` stores `gCamera`), pointer to the `0xDB4` object (`fn_8019578C`), counters at `+0x40` and `+0x404` | `+0x00` (1), texture pointer at `+0x0C` |
| `0xDBC` (`lbl_805FB670`, 0x20 bytes) | five words from `+0x04` (button state, `fn_801982A8`), two floats (`fn_801981FC`) | `+0x00` (1) |

`ScreenSetsInit` (0x8018C8F4) stores the vtable for the kind word in each font object the root
names, so the build-time address in the file is not used. The font object of slot `0xDB4` gets
no vtable from any code found so far; on the Wii it keeps the build-time address.

### Not converted yet

| Slot | Image offset | What is known | Why not |
|---|---|---|---|
| font data (font object `+0x08`) | `0x26740`, `0x38220`, `0x390E0`, `0x626C0` | glyph tables; `fn_8018979C` measures text with them, `fn_801971F0` (709 instructions) draws, `fn_801894F4` / `fn_801894B8` prepare a font loaded from a file | those functions are not decompiled; nothing on the boot path reads the data |
| sound objects named by action records | 42 objects between `0xDF2E0` and `0xF5D30` | argument 1 of `PlaySoundEffect`, argument 2 of `RegisterExperimentSound` and of `LT_SetTutorialText` (see "Action records") | their readers are not decompiled far enough to type the fields |
| `0xDC0` | `0xF6CF0` | a table of screen names (`UI_MSA_newscreen`, ...) with ids and hashes | no decompiled function reads the slot |
| `0x10`, `0x418`, `0x830`, `0x83C` | `0xF7580`, `0xF81D0`, `0xFDE20`, `0x100650` | effect data (`lightning_scrying`, `trail_plant`) | no decompiled function reads the slots |

With this the boot runs through `UiInit` (`ScreenSetsInit`, `ScreenMgrLoad`, `HudInit`,
`HudReset`), and the first `HudUpdateScreens` runs a state's enter action through the
interpreter. The screen format of `shared_ui.sir` (title screen) is expected to be the same; see
"Next".

## Found on the way

- `CAbstractInputTwoButtonToggle` (`fn_800208C0`, reset `fn_800208A0`) keeps state at `+0x50`,
  `+0x54` and `+0x58`, but the one toggle step in the file (image `0x3840`) is 0x50 bytes long
  and the next step starts at `0x3890`. Resetting or updating that toggle on the Wii overwrites
  the next step's first three words (raw value, value, source count). The loader converts those
  words as the next step's fields.
- The image's build-time `main.dol` addresses in the input steps are vtable slots that the boot
  code overwrites. That is one more class of them that needs no resolver.

- `PartDef` (`include/game/part.h`) ends with five `s16` at `0x40`–`0x49` and three `u8` at
  `0x4A`–`0x4C` according to the file's schema, not four `u16` and two `u8[4]`.
- The column-name hashes are not in `tools/hashes.py`'s dictionary (the names are not in the
  disc's strings). The reference names (`PART`, `ACTION`, ...) are. Guessing column names from
  the reference names would label most tables.

## Next

In the order the boot path needs them:

1. The **font glyph data** (first text drawn) and the **sound objects** the action records point
   at (first tutorial text or sound effect). For the second, and for any file whose image is not
   at its original address, the loader needs the script function table at load time: either the
   port converts those operands after the game has registered its functions, or a table of
   function index to signature is generated from a run of the PC build.
2. **`shared_ui.sir`** (title screen). `FixupFSMPoolCallback` (0x8010CCB0) and `HudReload` call
   the same `fn_801BC574` on machines from other files, so the state-machine schemas here should
   apply as they are; what is missing is how those files name their machines (the section type
   and the pool the callback walks), a relocation range per file, and the widget data the
   screens draw, which `global_binary.sir` does not have. Zone files have action tables of their
   own (`TriggerDoAction` takes the table from the zone record at `+0xD4` for a zone id other
   than 0x1FF): the same `{count, records}` and the same record format, so `ActionRecordPlan`
   should apply once those tables are found.
3. The effect data at `0x10`, `0x418`, `0x830`, `0x83C`, and **`shared_texture.sir`**: the texture
   header schema here should apply as it is.
4. **Save files**: written from memory by the Wii build, so big-endian too.
5. **`shared_parts.sir`** and the level and arena files: models, skeletons, textures.
