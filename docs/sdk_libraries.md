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
| 0x802581A4 | AXSetVoicePriority | moves the voice to the list of the new priority, stores it at voice+0x0C |
| 0x80258904 | AXRegisterAuxACallback | stores callback and context for aux bus A |
| 0x80258968 | AXRegisterAuxBCallback | same for aux bus B |
| 0x80259418 | AXSetMode | stores the output mode word |
| 0x80259B70 | AXRmtGetSamples | copies samples from the remote speaker ring of a channel |
| 0x80259C34 | AXRmtAdvancePtr | advances that ring's read position |
| 0x8025AC30 | AXSetVoiceSrcType | writes srcSelect / coefSelect (voice+0x30/+0x32), sync bit 0x1 |
| 0x8025ACF0 | AXSetVoiceState | writes the state (voice+0x38), sync bit 0x4 |
| 0x8025AD60 | AXSetVoiceType | writes the type (voice+0x3A), sync bit 0x8 |
| 0x8025ADAC | AXSetVoiceMix | copies 24 halfwords to voice+0x3C, builds the mixer control word |
| 0x8025B0A4 | AXSetVoiceAddr | copies loop flag, format and three addresses to voice+0x96; sets the gain for PCM formats |
| 0x8025B19C | AXSetVoiceAdpcm | copies 20 halfwords to voice+0xA6, sync bit 0x8000 |
| 0x8025B234 | AXSetVoiceSrc | copies 7 halfwords to voice+0xCE, sync bit 0x10000 |
| 0x8025B330 | AXSetVoiceAdpcmLoop | copies 3 halfwords to voice+0xDC, sync bit 0x40000 |
| 0x8025B3A0 | AXSetVoiceRmtOn | writes voice+0xFE, sync bit 0x800000 |
| 0x8026099C | MIXInit | fills the channel table (stride 0x70, one per AX voice) with defaults |
| 0x80260AB0 | MIXQuit | clears the table pointer |
| 0x80260AC4 | MIXSetSoundMode | stores the sound mode (0 mono .. 3 Pro Logic 2) |
| 0x80260AD4 | MIXInitChannel | nine arguments: voice, mode, input, three aux sends, pan, span, fader (tenths of a dB) |
| 0x80262194 | MIXReleaseChannel | clears the channel's voice pointer |
| 0x80262320 | MIXMute | sets the mute flag 0x8 |
| 0x80262344 | MIXUnMute | clears it |
| 0x80262368 | MIXIsMute | returns it |
| 0x80262384 | MIXSetFader | stores the fader, marks the channel changed |
| 0x802623A8 | MIXUpdateSettings | per frame: recomputes every live channel's mix and writes it to the voice |
| 0x80263F18 | AXARTInit | empties the sound list, sets the two 3D scales |
| 0x80263F4C | AXARTQuit | releases every sound's MIX channel |
| 0x80263FAC | AXARTServiceSounds | walks the sound list, evaluating each sound's articulators |
| 0x80263FEC | AXARTInitSound | stores voice and sample rate (as a float), empties the articulator list |
| 0x80264020 | AXARTAddSound | evaluates the articulators once, calls MIXInitChannel, links the sound |
| 0x80264420 | AXARTRemoveSound | unlinks the sound, releases its MIX channel |
| 0x80264498 | AXARTInitArt3D | articulator type 1 with defaults (pan 0x40, span 0x7F) |
| 0x80264508 | AXARTInitArtPanning | type 2 |
| 0x80264524 | AXARTInitArtPitch | type 5 |
| 0x80264538 | AXARTInitArtVolume | type 8 |
| 0x8026454C | AXARTInitArtAuxAVolume | type 9 |
| 0x80264560 | AXARTInitArtAuxBVolume | type 10 |
| 0x80264574 | AXARTInitArtAuxCVolume | type 11 |
| 0x80264588 | AXARTInitArtLpf | type 0x14 |
| 0x802645A8 | AXARTInitArtRmt | type 0x16 |
| 0x802645BC | AXARTInitArtRmtAuxVolume | type 0x18 (medium confidence on the name) |
| 0x80264A14 | AXARTAddArticulator | pushes an articulator on the sound's list |
| 0x80264A24 | AXARTRemoveArticulator | unlinks one |
| 0x80264CC8 | AXARTSet3DDopplerScale | stores the divisor of the 3D closing speed |
| 0x80264CD0 | AXARTSet3DDistanceScale | stores the divisor of the 3D distance |
| 0x802728A4 | WPADProbe | returns -1 / -2 per channel state, writes the extension type to the out pointer |
| 0x802727E0 | WPADDisconnect | clears a channel buffer; also called from the HBM home menu (medium confidence) |
| 0x80272AB0 | WPADSetDataFormat | compares the requested format with the current one, queues a command |
| 0x80273204 | WPADRead | reads the double-buffered status at +0xA0 (stride 0x60); called by the KPAD sampling callback |
| 0x8027D270 | KPADSetPosParam | stores two floats at channel+0x84/0x88 |
| 0x8027D28C | KPADSetHoriParam | stores two floats at channel+0x9C/0xA0 |
| 0x8027F504 | KPADRead | the game's per-frame read, per-channel stride 0x538 (medium confidence) |

Also named earlier: `WPADInit`, `WPADControlMotor`, `WPADSetConnectCallback`, `WPADSetExtensionCallback`,
`WPADSetSamplingCallback`, `WPADSetAutoSamplingBuf`, `WPADRegisterAllocator`, `KPADInit`, `AIInit`, `AXInit`.

## AX, MIX and AXART on PC

`src/port/port_audio.cpp` replaces these three libraries. The game uses no MIDI (SEQ / SYN are only
initialised): every sound is an AX voice. Music is two streamed `.dsp` files (left, right; DSP-ADPCM)
on voices of type 1, whose 0x10000-byte ring buffers the game refills from the disc while it reads
each voice's current address (voice+0xA2); sound effects are `.dsp` records in memory on voices of
type 0, set up by `fn_80203340`. Levels and pitch come from AXART articulators through MIX.
An AX address is a physical address in the format's units (ADPCM: nibbles, PCM16: samples, PCM8:
bytes). The AXFX effects on the aux buses (`0x8025B4C0`-`0x8025F284`) are not mixed.

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
- The PC port replaces the library (`src/port/port_bink.cpp`, decoder in `port_bink_codec.cpp`, derived from FFmpeg). From how `MoviePlay` uses them and the asm of three of them (not renamed in `symbols.txt`): `fn_802B47A0` BinkSetSoundTrack, `fn_802B4B30` BinkOpen, `fn_802B6C60` BinkSetVolume, `fn_802B4A1C` BinkGetFrameBuffersInfo (fills the `BINKFRAMEBUFFERS` the game allocates the Y/Cr/Cb plane textures from), `fn_802B4B14` BinkRegisterFrameBuffers (stores the pointer at `bink+0xB8`), `fn_802B6574` BinkGoto, `fn_802B5C6C` BinkDoFrame, `fn_802B61D0` BinkShouldSkip, `fn_802B625C` BinkNextFrame, `fn_802B6950` BinkWait, `fn_802B6798` BinkClose; `fn_802BE160` / `fn_802BE220` are the library's allocate (MSL `malloc` plus a 64-byte header) and free. The handle's first 0x20 bytes are width, height, frames, frame (1-based), last frame, rate, rate divisor, read error. Frames reach the screen through GX: planes are plain 8-bit rows that the game's TEV set-up (indirect lookup textures, `fn_80205894`) reads as I8 textures. The disc's movies are Bink 1 revision `i`, 640 wide and 520 (41 files), 480 (35) or 448 (2) high (`MoviePlay` centres the picture on the 640x480 screen, so the 520 ones lose 20 lines top and bottom, as on the console), 30 fps, five 44.1 kHz stereo DCT audio tracks (one per language).
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

## Round 2: OS, VI, DSP, GX TEV (17 names)

Only names the asm supports are given; everything else keeps its `fn_` name. Evidence per name:

| Library | Address | Name | Evidence |
|---|---|---|---|
| OS | 0x80235110 | DCStoreRange | `dcbst` loop, one store per 32-byte line from the aligned start |
| OS | 0x80234418 | OSCancelAlarm | disables interrupts, unlinks the alarm from `AlarmQueue`, re-arms the decrementer (`PPCMtdec`) |
| OS | 0x8023C15C | OSGetCurrentThread | returns the word at 0x800000E4 (current thread pointer) |
| VI | 0x802441F8 | VIConfigure | writes the display registers at 0xCC002000+ from a render-mode struct, delay loop, takes `fn_802440E0` table lookup |
| VI | 0x80245B08 | VIGetCurrentLine | reads the vertical count registers 0xCC00202C/202E, halves the line |
| VI | 0x80245BF8 | VIGetDTVStatus | returns bit 0 of 0xCC00206E |
| DSP | 0x80264FD8 | DSPInit | RVL_SDK - DSP banner (`OSRegisterVersion`), sets the DSP interrupt handler |
| DSP | 0x80264F90 | DSPCheckMailToDSP | top bit of the halfword at 0xCC005000 |
| DSP | 0x80264FA0 | DSPCheckMailFromDSP | top bit of the halfword at 0xCC005004 |
| DSP | 0x80264FB0 | DSPReadMailFromDSP | 32-bit read of 0xCC005004/0xCC005006 |
| DSP | 0x80264FC4 | DSPSendMailToDSP | 32-bit write to 0xCC005000/0xCC005002 |
| GX | 0x8024D59C | GXSetTevColorIn | FIFO 0x61 + TEV colour-env BP word (shadow 0x180); four 4-bit fields |
| GX | 0x8024D5DC | GXSetTevAlphaIn | same, alpha-env shadow (0x1C0), four 3-bit fields |
| GX | 0x8024D61C | GXSetTevColorOp | colour-env shadow (0x180), op/bias/scale/clamp/dest fields |
| GX | 0x8024D674 | GXSetTevAlphaOp | alpha-env shadow (0x1C0), same fields |
| GX | 0x8024D6CC | GXSetTevColor | two BP writes to 0xE0/0xE1 (TEV colour registers) from a 4-byte colour |
| GX | 0x8024D72C | GXSetTevColorS10 | as above, from an 8-byte S10 colour |

Notes:
- The GX setters write through `lis rX, 0xCC01` + `stb 0x61` at `-0x8000(rX)`, i.e. the write-gather pipe at 0xCC008000.
- Several GX and OS functions in the same ranges were checked and left as `fn_` (e.g. 0x8023D500, 0x8023D3C8, the TEV swap and indirect setters at 0x8024D7EC-0x8024D944, 0x8024DF28-0x8024E08C), because their asm did not identify them.
- Round 2 is well short of the 100 target: most of the OS (~800 unnamed functions in 0x80232C5C-0x8026FFFF), VI and GX functions have no direct evidence yet (no banner strings, few asserts). Next steps: identify the GX setters with the callers in the game range, and OS thread/message-queue functions via their callers.

## Signature matches (`tools/sig_match.py`)

484 more functions are named by matching `main.dol` against object code compiled from the public
[doldecomp/ogws](https://github.com/doldecomp/ogws) source (CC0-1.0; RVL SDK, MSL, MetroTRK, runtime),
built with Metrowerks `GC/3.0a5.2`. `ogws` is Wii Sports (2006 SDK), so only code that did not change
by 2008 matches; the rest stays `fn_`. The tool masks relocated fields (branch targets, `@ha/@l`,
`@sda21`) in each object function, then requires an equal size and equal unmasked bits, a unique
object function per DOL function and the reverse, no existing name, and call targets that do not
contradict names already known. Sources and objects are not in the repo. Names are in
`names_sdk.txt` under `# sigmatch <library>`; a trailing comment gives the source object file.
Functions of 12 bytes or less are not matched. Static functions are named only when no other
object has the same name.

| Library | Names | | Library | Names |
|---|---|---|---|---|
| OS | 68 | | NWC24 | 13 |
| BTE (Bluetooth stack) | 80 | | SC | 16 |
| GX | 53 | | FS | 16 |
| DVD | 36 | | IPC | 14 |
| AX | 33 | | MSL | 12 |
| WPAD | 32 | | VF | 9 |
| NAND | 28 | | ARC | 8 |
| WUD | 28 | | DSP | 7 |
| MEM | 5 | | USB | 4 |
| AI, EUART, EXI, runtime | 3 each | | SI, MTX, TPL | 2 each |
| AXFX, DB, WENC, MetroTRK | 1 each | | | |

Spot check: 20 random matches were compared with their callees; every call target the DOL
already named agreed with the object's callee name, and a check over all matches found 304 such
cross-calls and 0 disagreements.

### Second pass: mkw and tp (496 more names)

Same tool, two more public sources, compiled with the same MWCC `GC/3.0a5.2` and each project's own flags
(sources and objects stay outside the repo):

| Source | Licence | Used for |
|---|---|---|
| [doldecomp/mkw](https://github.com/doldecomp/mkw) (Mario Kart Wii, 2008 SDK) | CC0-1.0; `lib/gamespy` also carries GameSpy's own `license.txt`, only compiled and byte-matched here, nothing copied | GameSpy, SO, DWC, MSL, MTX, MEM, OS, ARC, IPC |
| [zeldaret/tp](https://github.com/zeldaret/tp) (Twilight Princess Wii, `libs/revolution`, MSL_C, TRK) | CC0-1.0 | GX, DVD, VI, AXFX, ESP, MSL, OS, MetroTRK, HBM, NAND, FS, WPAD |

Names added (sections `# sigmatch mkw <lib>` and `# sigmatch tp <lib>` in `names_sdk.txt`):

| Library | Names | | Library | Names |
|---|---|---|---|---|
| GameSpy (GP, serverbrowsing, gt2, gstats, qr2, darray, hashtable, md5) | 350 | | GX | 31 |
| MSL | 33 | | OS | 12 |
| DVD | 14 | | SO (socket layer) | 11 |
| VI | 10 | | MTX | 8 |
| AXFX | 7 | | MEM | 5 |
| ESP | 4 | | DWC | 2 |
| HBM, MetroTRK | 2 each | | ARC, IPC, FS, NAND, WPAD | 1 each |

Notes:
- GameSpy is linked into `main.dol` (the DWC online layer sits on it), so its names are real library names.
- The GameSpy socket wrappers (`socket`, `bind`, `connect`, `recv`, `recvfrom`, `sendto`, `getsockname`,
  `inet_addr`) are named `gs_<name>` so they cannot clash with Winsock in the PC build. `floor`/`tan` and
  the other math intrinsics stay `fn_`.
- The `names_sdk.txt` comment of each line gives the source object file. A cross-call check over all
  1416 named-callee pairs found 0 disagreements (the only differences are the `gs_` prefixes).
- Still `fn_`: NHTTP, SSL, NCD, NWC24 beyond ogws, RevoEX 2.1plus (no public source found in mkw, tp,
  Melee or NSMBW; NSMBW ships headers only), HBM beyond 2 (tp's `homebuttonLib` is mostly NonMatching and
  does not compile with this MWCC), MetroTRK beyond 3 (tp's TRK compiles 11/30 files with `3.0a5.2`).
- Melee (no licence declared) and NSMBW (no licence declared) were cloned for inspection and gave no names.
