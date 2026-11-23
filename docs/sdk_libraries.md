# SDK library address ranges (RM7P4F)

Ranges are approximate (from `<< RVL_SDK - X` banners, source-file strings and the named functions
in `config/RM7P4F/symbols.txt`). Names live in `config/RM7P4F/names_sdk.txt`, one `# Library` section each.

| Library | Range | Evidence |
|---|---|---|
| zlib | 0x80230680-0x80232AF4 | `inflate*` names |
| OS (PPC, arena, alarm, context, interrupt, thread, reset, SRAM, state) | 0x80232AF8-0x8023F9xx | `OSStateTM.c`, `OSLaunch.c`, named OS* neighbours |
| EXI / SI / DB | 0x80240790-0x80243800 | named `EXI*`, `SI*`, `DB*` |
| VI | 0x80243xxx-0x80247C24 | banner at 0x80244ED0, `VIInit` |
| GX | 0x80247C24-0x8024EE38 | banner at 0x80248DEC, `GXInit`, `__GXInitGX` |
| DVD (fs + core + low) | 0x8024EE48-0x80257560 | `DVDInit`, `DVDLow*`, `(DVDLowFinish)` string |
| AI / AX | 0x802577E4 / 0x80257C34 | `AIInit`, `AXInit` |
| DSP | 0x80264FD8 | banner |
| MEM / WPAD / KPAD | 0x80260xxx / 0x80270948 / 0x8027FC30 | names, banners |
| HBM (Home Button Menu) | 0x802C8FC8-0x802CC440+ | four banners |
| DWC / NHTTP | 0x802E49B8 / 0x8032C5B8 | banners |

## Named by this pass (33)

- GX texture objects: `GXInitTexObj*`, `GXGetTexObj*`, `GXLoadTexObj*` (0x8024C3B8-0x8024C91C).
  Evidence: the init function memsets a 0x20-byte object and packs fields the getters read back.
- GX pixel engine direct pokes (`GXPoke*`, 0x8024B418-0x8024B4F0, alphabetical API order, peReg writes),
  `GXReadDrawSync`, draw-sync/done callbacks, token/finish interrupt handlers, `__GXSetDirtyState` (0x8024B6FC).
- DVD fs: `DVDConvertPathToEntrynum`, `DVDFastOpen`, `DVDOpen`, `DVDClose`, `entryToPath`
  (path parsing on '/' '.', recursive entry walk).

Unnamed GX functions between 0x8024D508 and 0x8024EE38 (texture/TEV/lighting) and the DVD queue code
0x8024F96C-0x802536C8 are still `fn_`; best guesses need per-function asm review.
