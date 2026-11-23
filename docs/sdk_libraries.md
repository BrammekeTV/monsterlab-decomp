# SDK libraries: input and audio

Partial map of `main.dol`'s RVL SDK libraries (WPAD, KPAD, SI, AX, AI, DSP). Ranges are approximate:
they come from the `<< RVL_SDK - X >>` banner, the position of the library's init function and the
call graph. Names are in `config/RM7P4F/names_sdk.txt` (sections `# WPAD/KPAD/SI` and `# AX/AI/DSP`).
Names follow the public RVL SDK API; a name is only given when the asm shows it (hardware register
use, argument use, call pattern). Everything else keeps its `fn_` name.

| Library | Range (approx.) | Anchors |
|---|---|---|
| DVDLow | 0x80255270-0x80257700 | `DVDLowInit`, IOS ioctl wrappers (not covered here) |
| AI | 0x80257700-0x80257C34 | `AIInit` (0x802577E4); hardware regs 0xCC005030-0xCC00503A |
| AX | 0x80257C34-~0x80260500 | `AXInit` (0x80257C34), voice list at 0x80563900/0x80563980; AXFX-style effect blocks follow from ~0x8025B65C |
| game synth/streaming glue | 0x80263F18-0x80264CD0 | called from the game audio code (0x801FD420, 0x801FE52C); unidentified |
| DSP | 0x80264EE8-~0x80265800 | banner at 0x80264FD8, "DSP is booting task" string at 0x80265834 |
| WPAD | 0x80270948-0x8027D220 | banner at 0x80270948/0x80272204, `WPADInit` (0x80271358); WUD (0x802858B4) lies after it |
| KPAD | 0x8027D220-0x8027FFFF | banner string at 0x803A1AF0, `KPADInit` (0x8027FC30), channel struct stride 0x538 at 0x8058CC20 |
| SI | not located | banner at 0x80397300 (data); no code found yet |

## Named functions

| Address | Name | Evidence |
|---|---|---|
| 0x802576BC | AIRegisterDMACallback | swaps a global under OSDisableInterrupts, returns the old value |
| 0x80257700 | AIInitDMA | writes start address / length to 0xCC005030/32/36 |
| 0x8025777C | AIStartDMA | sets enable bit 0x8000 of 0xCC005036 |
| 0x80257790 | AIStopDMA | clears that bit |
| 0x802577A4 | AIGetDMABytesLeft | reads 0xCC00503A, x32 |
| 0x802577B4 | AIGetDMAStartAddr | combines 0xCC005030/32 |
| 0x802577CC | AIGetDMALength | reads 0xCC005036 low 15 bits, x32 |
| 0x80257C88 | AXQuit | placed directly after `AXInit`; only called from the audio shutdown 0x801FD1F0 |
| 0x80257FA8 | AXFreeVoice | unlinks a voice from the active list, returns it to the free list (0x80563900) |
| 0x80258024 | AXAcquireVoice | pops the free voice list, takes priority/callback/context arguments |
| 0x80259B2C | AXRegisterCallback | swap-under-interrupts, returns the old callback; game registers 0x80201F38 |
| 0x802728A4 | WPADProbe | returns -1 / -2 per channel state, writes the extension type to the out pointer |
| 0x802727E0 | WPADDisconnect | clears a channel buffer; also called from the HBM home menu (medium confidence) |
| 0x80272AB0 | WPADSetDataFormat | compares the requested format with the current one, queues a command |
| 0x80273204 | WPADRead | reads the double-buffered status at +0xA0 (stride 0x60); called by the KPAD sampling callback |
| 0x8027D270 | KPADSetPosParam | stores two floats at channel+0x84/0x88 |
| 0x8027D28C | KPADSetHoriParam | stores two floats at channel+0x9C/0xA0 |
| 0x8027F504 | KPADRead | the game's per-frame read, per-channel stride 0x538 (medium confidence) |

Also named earlier: `WPADInit`, `WPADControlMotor`, `WPADSetConnectCallback`, `WPADSetExtensionCallback`,
`WPADSetSamplingCallback`, `WPADSetAutoSamplingBuf`, `WPADRegisterAllocator`, `KPADInit`, `AIInit`, `AXInit`.

## Called by the game (still `fn_`)

Game code (0x80006B40-0x8022FEC4) also calls: AX voice/parameter setters 0x8025AC30-0x8025B3A0 and
0x80259B70/0x80259C34, AXFX effect init/callback at 0x8025B65C-0x8025F284, 0x802737A8 (large WPAD
command function), 0x80274F80/0x80275004 (WPAD speaker), 0x8027FF24 (KPAD, resets motors on 4 channels),
0x80264020-0x80264A24 (synth glue). Their signatures are what the PC audio/input layer must provide.

# SDK and middleware libraries in main.dol

Map of the library code above the game range (`0x80006B40`–`0x8022FEC4`). Ranges come from
the release banners and source-file strings in `main.dol`, and from the call graph
(`tools/xref.py`). Boundaries are estimates unless marked **exact**. Names are in
`config/RM7P4F/names_sdk.txt`. The `# NET` section there holds the names applied in this pass.

Mark key: **stub** = no-op or error return on PC. **C lib** = map to the C library.
**replace** = needs a PC implementation.

## Ranges

| Library | Code range (approx.) | Banner / evidence | PC mark |
|---|---|---|---|
| zlib 1.2.3 | `0x8022FEC4`–`0x80232C5C` | `crc32` `0x8022FEFC`, `inflate*` `0x802309E0`–`0x802326C0`; already named | **replace**: `third_party/zlib` (port already uses real zlib, see `docs/port_runtime.md`) |
| Bink (video) | `0x802B3020`–`0x802BE300` (probable) | `Not a Bink file.` `0x8034AFA4`, `Error reading Bink header.` `0x8034B017`, `Out of memory.`; `TPL.c` string sits at `0x802B3D54` inside this block, so the start is uncertain | **replace**: PC video decoder (game code entry is `MoviePlay`) |
| DWC (friends / matchmaking, RVL) | `0x802E4000`–`0x802F841C` | banner `0x802E49B8`, `<< RVL_SDK - DWC ... Mar 7 2008 >>` `0x803AD138`, `DWC_*` debug strings; `dwc_auth_interface.c` `0x803B091C`, `dwci_nd.c` `0x803B3B94` | **stub** |
| DWC auth + NHTTP (interleaved) | `0x802F841C`–`0x802FA3EC` | `/shared2/DWC_AUTHDATA` `0x802F841C`, `NHTTPDestroyResponse` `0x802F85C4`, `DWCi_Auth_EndProcess` `0x802FA394` | **stub** |
| NHTTP (HTTP client) | `0x802F85C4`–`0x8032C800` (contains parts of NCD/NWC24 area) | banner `0x8032C5B8` (`Oct 11 2007`, `UNOFFICIAL` variant at `0x803B4480`); `NHTTP_bgnend.c` `0x803B41F8`, `NHTTP_os_RVL.c` `0x803B42E8`, `d_nhttp.c` `0x803B467C` | **stub** |
| NCD (network config) | `0x80320D10`–`0x80325ED4` | banner `0x803B3DA8` (`Dec 10 2007`), `ncdsystem.c` `0x803B3E50`, `/dev/net/ncd/manage` | **stub** |
| NWC24 (Wii Connect 24) | `0x8032292C`–`0x80323914`+ | banner `0x803B3E98`, `/dev/net/kd/request`, `/dev/net/kd/time`, `/dev/net/ip/top` | **stub** |
| SSL | `0x8032E18C`–`0x8032EE10` | banner `0x803B4920` (`Nov 6 2007`), `/dev/net/ssl`, `ssl error(%d)` `0x803B0BB8` (used by DWC) | **stub** |
| SO / SOCKET | not located | banners `0x803B3FA0` (SO) and `0x803B4180` (SOCKET); error table `SO_*` at `0x803871D4`; socket error text used by DWC (`0x803B1470`–`0x803B3B94`) | **stub** (the `SO_SUCCESS` code at `0x80207xxx` is game-side wrapping, not SO) |
| MSL C runtime (Metrowerks CW) | `0x8032EEE4`–`0x80340000` | `strlen` `0x8032EEE4`, `__cvt_fp2unsigned` `0x8032F400`, `__destroy_arr` `0x8032F2E0`, `GCN_Mem_Alloc.c` banner `0x80352220`, `snprintf` `0x80336E38`, `vsprintf` `0x80336DB8`, `strcmp` `0x80338910`, `rand` `0x80337150`, `sinf` `0x8033F0A4`, `cosf` `0x8033EC98`; `memcpy` / `memset` at `0x80004338` / `0x8000443C` (CRT, in `.init`) | **C lib** |
| MetroTRK (debugger, RVL) | `0x80340000`–`0x80345AB8` | banner `MetroTRK for Revolution v0.1` `0x80353248`, `Metrowerks Target Resident Kernel for PowerPC` `0x80344314`; `InitMetroTRK` `0x8034419C`, `InitMetroTRK_BBA` `0x80344234`, `InitMetroTRKCommTable` `0x803447B0` | **stub** |

Other SDK code (OS, EXI, SI, VI, GX, DVD, AI/AX, DSP, NAND, SC, WPAD, KPAD, HBM, TPL, WUD, arc)
is in `0x80232C5C`–`0x802B2xxx` and is not covered here; `names_sdk.txt` has its existing names.

Overlaps: DWC auth and NHTTP are interleaved, and NCD/NWC24 sit inside the NHTTP range. The
boundaries above are the outermost addresses seen, not a single contiguous range per library.

## Game entry points into each library

Callers are game-range functions (below `0x8022FEC4`) that call into the library range. Only named callers are listed.

### Bink
- `MoviePlay` (`0x802049E0`) calls `0x802B47A0`, `0x802B4A1C`, `0x802B4B14`, `0x802B4B30`, `0x802B5C6C`, `0x802B61D0`, `0x802B6574`, `0x802B6798`, `0x802B6950`, `0x802B6C60`, `0x802B625C`.
- Probable open path: `fn_802B4B30` (4124 bytes, `Error opening file.`, `memset`/`memcpy`, called only from `MoviePlay`).
- `fn_802BBF18` (7792 bytes, jump tables, `@ABC...` tables at `0x8034B060`) looks like the frame decoder; not verified.
- `0x80204478`, `0x80205454`, `0x8020506C` also call into `0x802B4140`, `0x802BE160`, `0x802BE220`.

### DWC
- Init / teardown: `fn_8020844C` calls `0x802E49B8` (banner function), `MultiplayerCleanup` calls `0x802E4970`, `KillMatchConnection` calls `0x802E9CBC`.
- Friends: `FriendListDelete` calls `0x802E65C0` and `0x802E99C4`; `FriendsListAddFriend` and `LoginPoller` call `0x802E99C4`; `FriendListSort` / `IsListFull` call `0x802E64A0`.
- Matchmaking: `SendMatchupRequest` calls `0x802E6528` and `0x802E9AB0`.

### NHTTP / NCD / NWC24 / SSL (called through DWC or game wrappers)
- `FriendsListIsBuddy` → `0x802FA554`; `FriendCodeInputVerify` → `0x802FA5BC`, `0x802FB094`; `FriendListInit` → `0x802FB094`; `FriendsListAddFriend` → `0x802FABA4`, `0x802FB13C`.
- `fn_80208A54` → `0x80320968`, `0x8032D3C0`; `fn_802078E8` → `0x80323644`, `0x80323908`; `fn_8020852C` → `0x8032380C`, `0x80323D3C`.

### MSL
- Most game calls are to `strlen`, `strcmp`, `memcpy`, `memset`, `snprintf`, `sinf`, `cosf`, `rand` and `__cvt_fp2unsigned`.
- `__destroy_arr` and the array helper at `0x8032F1E8` are called from the save manager (`SaveManager_LoadData`, `SaveManager_EraseData`, …) and `BattleUpdate`.

### MetroTRK
- `__start` calls `InitMetroTRK_BBA`; `fn_8000446C` calls `0x80341D64`.

## Names applied

Applied through `tools/rename.py -f config/RM7P4F/names_sdk.txt` (the `# NET` section, from
debug strings that repeat the function name):

- `NHTTPi_CheckCurrentThread` (`0x8032683C`)
- `NCDGetCurrentIfConfig` (`0x80320D10`), `NCDGetCurrentIpConfig` (`0x80320E08`), `NCDiGetEnabledConfigList` (`0x8032118C`)
- `NWC24iSetScriptMode` (`0x8032292C`), `NWC24iRequestGenerateUserId` (`0x80322ACC`), `NWC24iSetRtcCounter` (`0x80323164`), `NWC24iRequestShutdown` (`0x80323574`)
- `DWCi_MatchInit` (`0x802EB5C4`)

No new MSL or Bink names. `strlen`, `strcmp`, `snprintf`, `sinf`, `cosf`, `rand` and `memcpy`/`memset` were already named. `fn_8032F1E8` is a C++ array construct/destroy helper, not a public API, so it stays `fn_`. `fn_802B4B30` is the likely Bink open path but is not named: the evidence is one error string and the single `MoviePlay` caller. Naming it `BinkOpen` needs its asm checked against the Bink API.

## Pending

- The `# MSL` and `# Bink` sections of `names_sdk.txt` are empty for now (see above).
- SO/SOCKET code is not located.
- `dtk dol split` was not re-run in this pass, so `build/` asm still shows the old `fn_` names for the renamed functions.

## GX/DVD names added in PR #85 (33)

- GX texture objects: `GXInitTexObj*`, `GXGetTexObj*`, `GXLoadTexObj*` (0x8024C3B8-0x8024C91C).
- GX pixel engine pokes `GXPoke*` (0x8024B418-0x8024B4F0), `GXReadDrawSync`, token/finish interrupt handlers, `__GXSetDirtyState` (0x8024B6FC).
- DVD fs: `DVDConvertPathToEntrynum`, `DVDFastOpen`, `DVDOpen`, `DVDClose`, `entryToPath`.
- Left as `fn_`: 0x8024B50C/0x8024B5D4 (draw sync/done callbacks, defined in port_gx.cpp).
