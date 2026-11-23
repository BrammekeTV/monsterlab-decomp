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
