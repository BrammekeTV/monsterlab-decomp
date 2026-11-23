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
