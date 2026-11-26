// PC port: savestates. --save-state FILE@FRAME writes the whole running game to a file at the end of that
// frame; --load-state FILE starts a new process from it instead of booting. No original code.
// Design and limits: docs/savestates.md.
//
// What a state holds:
//   - the mapped Wii memory (MEM1, MEM2, the hardware register page),
//   - the stacks of the game threads, which live at fixed addresses for this (PORT_STACK_BASE), and the
//     registers each thread had where it waits,
//   - the sections .savd/.savb of the exe: every static of the game code and the port's emulated-console
//     state (OS, DVD, VI, GX registers, AX voices, input, scheduler),
//   - the port's fixed-address heap (PortHostAlloc) and a few named blocks (the renderer's palette memory).
// Stacks and statics hold code addresses of this exe, so a state belongs to the exact exe that wrote it:
// the header carries a hash of the exe and of main.dol and a state made by another build is refused.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <compressapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "port/port.h"

#pragma comment(lib, "cabinet.lib")

BOOL PortNandBusy(void);   // port_nand.cpp
const char* PortNandRoot(BOOL* isTestFolder);
u32  PortGfxStateSave(u8** block);          // port_gfx_d3d11.cpp: the frame buffer and its copies
void PortGfxStateLoad(const u8* p, u32 n);

static const u32 kMagic = 0x54534C4D;      // "MLST"
static const u32 kVersion = 1;
enum { CHUNK_MEM = 1, CHUNK_SECTION = 2, CHUNK_BLOB = 3, CHUNK_END = 4, CHUNK_FILE = 5, CHUNK_GFX = 6 };   // FILE: a file of the save folder, stored as it is
static const u32 kPiece = 0x400000;        // memory is packed in pieces of this size

struct Header {
    u32  magic, version;
    u32  buildHash;        // CRC-32 of the exe file
    u32  dataHash;         // CRC-32 of main.dol
    s32  frame;
    u32  width, height;    // the output size of the run that saved (information only)
    u32  flags;            // bit 0: saved while fast-forwarding
    char built[32];        // when the exe was linked, for the message
    PortCtx ctx[PORT_STACK_SLOTS];
    u8   active[PORT_STACK_SLOTS];
    u8   pad[2];
};
struct Chunk { u32 kind, addr, size, packed; char name[16]; };   // packed 0: all zero, nothing stored

struct Request { char path[400]; s32 frame; };
static Request sSave[128];
static s32     sSaveCount;
static char    sLoadPath[400];
static BOOL    sLoaded;
struct Blob { const char* name; void* p; u32 size; };
static Blob sBlobs[16];
static s32  sBlobCount;
static void (*sOnLoad[16])(void);
static s32  sOnLoadCount;
static void* sHostEsp[PORT_STACK_SLOTS];   // each thread's own Windows stack pointer while it runs on a slot stack
static void* sHostTib[PORT_STACK_SLOTS][3];

// ------------------------------------------------------------------ contexts and stacks
// setjmp/longjmp without the unwinding: the callee-saved registers, the stack pointer, the return address,
// the exception chain and the floating-point control words.
extern "C" __declspec(naked) int __cdecl PortCtxCapture(PortCtx* c) {
    __asm {
        mov eax, [esp + 4]
        mov [eax]PortCtx.ebp, ebp
        mov [eax]PortCtx.ebx, ebx
        mov [eax]PortCtx.esi, esi
        mov [eax]PortCtx.edi, edi
        lea ecx, [esp + 4]
        mov [eax]PortCtx.esp, ecx            // as after the return
        mov ecx, [esp]
        mov [eax]PortCtx.eip, ecx
        mov ecx, fs:[0]
        mov [eax]PortCtx.seh, ecx
        fnstcw word ptr [eax]PortCtx.fcw
        stmxcsr [eax]PortCtx.mxcsr
        xor eax, eax
        ret
    }
}
extern "C" __declspec(naked) void __cdecl PortCtxResume(const PortCtx* c) {
    __asm {
        mov eax, [esp + 4]
        fninit
        fldcw word ptr [eax]PortCtx.fcw
        ldmxcsr [eax]PortCtx.mxcsr
        mov ecx, [eax]PortCtx.seh
        mov fs:[0], ecx
        mov ebp, [eax]PortCtx.ebp
        mov ebx, [eax]PortCtx.ebx
        mov esi, [eax]PortCtx.esi
        mov edi, [eax]PortCtx.edi
        mov esp, [eax]PortCtx.esp
        mov ecx, [eax]PortCtx.eip
        mov eax, 1
        jmp ecx
    }
}

u32 PortStackTop(s32 slot) { return PORT_STACK_BASE + (u32)(slot + 1) * PORT_STACK_SLOT; }

// The thread information block's stack bounds and exception chain: Windows checks exception frames against
// the bounds, and the chain on a slot stack must not lead back to the thread's own stack.
static void TibEnter(s32 slot, u32 size) {
    NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
    sHostTib[slot][0] = tib->StackBase;
    sHostTib[slot][1] = tib->StackLimit;
    sHostTib[slot][2] = tib->ExceptionList;
    tib->StackBase = (void*)(uintptr_t)PortStackTop(slot);
    tib->StackLimit = (void*)(uintptr_t)(PortStackTop(slot) - size);
    tib->ExceptionList = (EXCEPTION_REGISTRATION_RECORD*)(uintptr_t)0xFFFFFFFF;
}
static void __cdecl TibLeave(s32 slot) {
    NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
    tib->StackBase = sHostTib[slot][0];
    tib->StackLimit = sHostTib[slot][1];
    tib->ExceptionList = (EXCEPTION_REGISTRATION_RECORD*)sHostTib[slot][2];
}
static BOOL CommitStack(s32 slot, u32 size) {
    return VirtualAlloc((void*)(uintptr_t)(PortStackTop(slot) - size), size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

// Switches to the slot's stack, calls fn(arg) there and comes back. The frame it builds at the top of the slot
// stack is the same in every process of this exe, so a thread a loaded state continues can return through it.
static __declspec(naked) void __cdecl StackCall(s32 slot, u32 top, void (__cdecl* fn)(void*), void* arg) {
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov esi, [esp + 20]                  // slot
        mov eax, [esp + 24]                  // top
        mov ecx, [esp + 28]                  // fn
        mov edx, [esp + 32]                  // arg
        mov sHostEsp[esi * 4], esp
        mov esp, eax
        push esi                             // kept on the slot stack for the way back
        push edx
        call ecx
        add esp, 4
        pop esi
        push esi
        call TibLeave
        add esp, 4
        mov esp, sHostEsp[esi * 4]
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
// A new thread takes the place of a saved one: its own stack pointer is remembered for the day the thread
// function returns (through StackCall's tail above), then it continues the saved registers.
static __declspec(naked) void __cdecl StackContinue(s32 slot, const PortCtx* c) {
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov esi, [esp + 20]
        mov eax, [esp + 24]
        mov sHostEsp[esi * 4], esp
        push eax
        call PortCtxResume
    }
}

void PortStackEnter(s32 slot, u32 size, void (__cdecl* fn)(void*), void* arg) {
    if (!CommitStack(slot, size)) PortStop(7, "cannot map the stack of game thread %d", slot);
    TibEnter(slot, size);
    StackCall(slot, PortStackTop(slot), fn, arg);
}
void PortStackResume(s32 slot, const PortCtx* c) {
    TibEnter(slot, slot == 0 ? 0x400000 : 0x100000);
    StackContinue(slot, c);
}

void PortStateInit(void) {
    void* s = VirtualAlloc((void*)(uintptr_t)PORT_STACK_BASE, PORT_STACK_SLOT * PORT_STACK_SLOTS, MEM_RESERVE, PAGE_READWRITE);
    void* h = VirtualAlloc((void*)(uintptr_t)PORT_HOSTMEM_BASE, PORT_HOSTMEM_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!s || !h) {
        fprintf(stderr, "[port] cannot map the game thread stacks at 0x%08X (error %lu)\n", PORT_STACK_BASE, GetLastError());
        ExitProcess(10);
    }
}

// ------------------------------------------------------------------ what the exe is
u32 PortCrc32(u32 crc, const u8* p, size_t n);
static u32 Crc32(u32 crc, const u8* p, size_t n) { return PortCrc32(crc, p, n); }
u32 PortCrc32(u32 crc, const u8* p, size_t n) {
    static u32 table[256];
    if (!table[1]) for (u32 i = 0; i < 256; i++) { u32 c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; table[i] = c; }
    crc = ~crc;
    while (n--) crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static u32 BuildHash(char* built, u32 n) {
    static u32 hash;
    static char when[32];
    if (!hash) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        FILE* f = fopen(path, "rb");
        if (f) {
            static u8 buf[0x10000];
            size_t got;
            while ((got = fread(buf, 1, sizeof(buf), f)) > 0) hash = Crc32(hash, buf, got);
            fclose(f);
        }
        if (!hash) hash = 1;
        const u8* pe = (const u8*)GetModuleHandleA(NULL);
        time_t stamp = (time_t)*(const u32*)(pe + *(const u32*)(pe + 0x3C) + 8);     // the link time
        struct tm* t = localtime(&stamp);
        if (t) strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", t);
    }
    if (built) snprintf(built, n, "%s", when);
    return hash;
}

// The saved sections of this exe, by name.
static BOOL FindSection(const char* name, u8** start, u32* size) {
    const u8* base = (const u8*)GetModuleHandleA(NULL);
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(base + ((const IMAGE_DOS_HEADER*)base)->e_lfanew);
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (strncmp((const char*)sec[i].Name, name, 8)) continue;
        *start = (u8*)base + sec[i].VirtualAddress;
        *size = sec[i].Misc.VirtualSize;
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------ options
void PortStateAddArg(const char* opt, const char* value) {
    if (!strcmp(opt, "--load-state")) { snprintf(sLoadPath, sizeof(sLoadPath), "%s", value); return; }
    const char* at = strrchr(value, '@');
    if (!at || atoi(at + 1) <= 0 || sSaveCount >= 128) {
        fprintf(stderr, "[port] --save-state %s: expected FILE@FRAME (up to 128 of them)\n", value);
        return;
    }
    Request* r = &sSave[sSaveCount++];
    snprintf(r->path, sizeof(r->path), "%.*s", (int)(at - value), value);
    r->frame = atoi(at + 1);
}
const char* PortStateLoadPath(void) { return sLoadPath[0] ? sLoadPath : NULL; }
BOOL PortStateLoaded(void) { return sLoaded; }
void PortStateBlob(const char* name, void* p, u32 size) { if (sBlobCount < 16) sBlobs[sBlobCount++] = {name, p, size}; }
void PortStateOnLoad(void (*fn)(void)) { if (sOnLoadCount < 16) sOnLoad[sOnLoadCount++] = fn; }

// ------------------------------------------------------------------ writing
static COMPRESSOR_HANDLE sPack;
static u8* sPackBuf;

static BOOL WriteChunk(FILE* f, u32 kind, const char* name, const u8* p, u32 size) {
    for (u32 off = 0; off < size || (size == 0 && off == 0); off += kPiece) {
        u32 n = size - off < kPiece ? size - off : kPiece;
        Chunk c = {};
        c.kind = kind;
        c.addr = (u32)(uintptr_t)p + off;
        c.size = n;
        snprintf(c.name, sizeof(c.name), "%s", name);
        BOOL zero = TRUE;
        for (u32 i = 0; i < n && zero; i += 4) if (*(const u32*)(p + off + i)) zero = FALSE;   // sizes are multiples of 4
        SIZE_T packed = 0;
        if (!zero && !Compress(sPack, (PVOID)(p + off), n, sPackBuf, kPiece + 0x10000, &packed)) return FALSE;
        c.packed = (u32)packed;
        if (fwrite(&c, sizeof(c), 1, f) != 1) return FALSE;
        if (packed && fwrite(sPackBuf, 1, packed, f) != packed) return FALSE;
        if (size == 0) break;
    }
    return TRUE;
}

static u32 Align4(u32 n) { return (n + 3) & ~3u; }

// The files of the save folder (and of its tmp folder), each as one chunk named by its path in the folder.
static BOOL WriteSaveFiles(FILE* f) {
    BOOL test;
    const char* root = PortNandRoot(&test);
    static const char* const kDirs[] = {"", "tmp\\"};
    for (const char* dir : kDirs) {
        char pattern[MAX_PATH + 16];
        snprintf(pattern, sizeof(pattern), "%s\\%s*", root, dir);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char path[MAX_PATH + 300];
            snprintf(path, sizeof(path), "%s\\%s%s", root, dir, fd.cFileName);
            Chunk c = {};
            c.kind = CHUNK_FILE;
            if (snprintf(c.name, sizeof(c.name), "%s%s", dir, fd.cFileName) >= (int)sizeof(c.name)) continue;   // not a name the game uses
            FILE* in = fopen(path, "rb");
            if (!in) continue;
            static u8 data[0x40000];
            c.size = c.packed = (u32)fread(data, 1, sizeof(data), in);
            fclose(in);
            if (fwrite(&c, sizeof(c), 1, f) != 1 || fwrite(data, 1, c.size, f) != c.size) { FindClose(h); return FALSE; }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return TRUE;
}

// The save folder as the state has it: its files are deleted, then the state's are written (ReadSaveFile).
static void ClearSaveFiles(const char* root) {
    static const char* const kDirs[] = {"", "tmp\\"};
    for (const char* dir : kDirs) {
        char pattern[MAX_PATH + 16];
        snprintf(pattern, sizeof(pattern), "%s\\%s*", root, dir);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char path[MAX_PATH + 300];
            snprintf(path, sizeof(path), "%s\\%s%s", root, dir, fd.cFileName);
            DeleteFileA(path);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
}

static BOOL Save(const char* path, s32 frame) {
    if (PortBinkPlaying()) { PortLog("state: %s not written: a movie is playing (frame %d); run with --no-movies", path, frame); return FALSE; }
    if (PortNandBusy()) { PortLog("state: %s not written: the game has a save file open (frame %d)", path, frame); return FALSE; }
    Header h = {};
    h.magic = kMagic;
    h.version = kVersion;
    h.buildHash = BuildHash(h.built, sizeof(h.built));
    h.dataHash = PortDataHash();
    h.frame = frame;
    h.width = (u32)gPortSettings.width;
    h.height = (u32)gPortSettings.height;
    h.flags = PortFastActive() ? 1 : 0;
    PortClockFreeze();
    PortCtx* ctx[PORT_STACK_SLOTS] = {};
    s32 self = PortSchedSave(ctx, h.active);
    if (self < 0) { PortLog("state: %s not written: frame %d ended inside an interrupt of a real-time run; save in a --fast run", path, frame); return FALSE; }
    // The saving thread's own registers. In a process that loads the state this call returns 1.
    volatile const char* resumedPath = path;
    if (PortCtxCapture(ctx[self])) {
        (void)resumedPath;
        return TRUE;                                   // the loaded process continues here, in PortStateFrame
    }
    for (int i = 0; i < PORT_STACK_SLOTS; i++) if (h.active[i]) h.ctx[i] = *ctx[i];

    char tmp[420];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE* f = fopen(tmp, "wb");
    if (!f) { PortLog("state: cannot write %s", path); return FALSE; }
    if (!sPack) {
        CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &sPack);
        sPackBuf = (u8*)malloc(kPiece + 0x10000);
    }
    BOOL ok = sPack && sPackBuf && fwrite(&h, sizeof(h), 1, f) == 1;
    ok = ok && WriteChunk(f, CHUNK_MEM, "mem1", (const u8*)(uintptr_t)PORT_MEM1_BASE, PORT_MEM1_SIZE);
    ok = ok && WriteChunk(f, CHUNK_MEM, "mem2", (const u8*)(uintptr_t)PORT_MEM2_BASE, PORT_MEM2_SIZE);
    ok = ok && WriteChunk(f, CHUNK_MEM, "hw", (const u8*)(uintptr_t)PORT_HW_BASE, PORT_HW_SIZE);
    ok = ok && WriteChunk(f, CHUNK_MEM, "hostmem", (const u8*)(uintptr_t)PORT_HOSTMEM_BASE, PORT_HOSTMEM_SIZE);
    for (int i = 0; i < PORT_STACK_SLOTS && ok; i++) {
        if (!h.active[i]) continue;
        u32 lo = h.ctx[i].esp & ~0xFFFu;               // everything a waiting thread still needs lies above its stack pointer
        ok = WriteChunk(f, CHUNK_MEM, "stack", (const u8*)(uintptr_t)lo, PortStackTop(i) - lo);
    }
    static const char* const kSections[] = {".savd", ".savb"};
    for (const char* name : kSections) {
        u8* p; u32 n;
        if (!FindSection(name, &p, &n)) continue;
        ok = ok && WriteChunk(f, CHUNK_SECTION, name, p, Align4(n));
    }
    for (int i = 0; i < sBlobCount && ok; i++) ok = WriteChunk(f, CHUNK_BLOB, sBlobs[i].name, (const u8*)sBlobs[i].p, sBlobs[i].size);
    ok = ok && WriteSaveFiles(f);
    u8* gfx = NULL;
    u32 gfxSize = ok ? PortGfxStateSave(&gfx) : 0;     // pieces of one block: the first has the whole size in its name
    if (gfxSize) {
        char total[16];
        snprintf(total, sizeof(total), "%u", gfxSize);
        ok = WriteChunk(f, CHUNK_GFX, total, gfx, Align4(gfxSize) <= gfxSize ? gfxSize : gfxSize & ~3u);
    }
    free(gfx);
    Chunk end = {};
    end.kind = CHUNK_END;
    ok = ok && fwrite(&end, sizeof(end), 1, f) == 1;
    long bytes = ftell(f);
    ok = fclose(f) == 0 && ok;
    if (ok) ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING);
    if (!ok) { PortLog("state: writing %s failed", path); DeleteFileA(tmp); return FALSE; }
    PortLog("state: saved frame %d to %s (%.1f MB, build %08X of %s)", frame, path, bytes / 1048576.0, h.buildHash, h.built);
    return TRUE;
}

void PortStateFrame(s32 frame) {
    for (s32 i = 0; i < sSaveCount; i++) if (sSave[i].frame == frame) Save(sSave[i].path, frame);
}

// ------------------------------------------------------------------ reading
static void Refuse(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[port] state: %s cannot be loaded: ", sLoadPath);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    fflush(stderr);
    ExitProcess(13);
}

static PortCtx sResume[PORT_STACK_SLOTS];
static u8      sResumeActive[PORT_STACK_SLOTS];
const PortCtx* PortStateCtx(s32 slot) { return sResumeActive[slot] ? &sResume[slot] : NULL; }

void PortStateLoad(void) {
    FILE* f = fopen(sLoadPath, "rb");
    if (!f) Refuse("no such file");
    Header h;
    if (fread(&h, sizeof(h), 1, f) != 1 || h.magic != kMagic) Refuse("not a Monster Lab state file");
    if (h.version != kVersion) Refuse("it is state format %u, this exe reads format %u. Make the states again (tools/playthrough.py --refresh)", h.version, kVersion);
    char built[32];
    u32 build = BuildHash(built, sizeof(built));
    if (h.buildHash != build)
        Refuse("it was made by another build (%08X, linked %s) than this exe (%08X, linked %s). A state holds this exe's code addresses, "
               "so it only loads into the exe that wrote it. Make the states again (tools/playthrough.py --refresh)",
               h.buildHash, h.built, build, built);
    if (h.dataHash != PortDataHash())
        Refuse("it was made with other game data (main.dol %08X, now %08X)", h.dataHash, PortDataHash());
    if ((h.flags & 1) && !PortFastActive())
        fprintf(stderr, "[port] state: saved while fast-forwarding, continuing in real time\n");
    DECOMPRESSOR_HANDLE unpack = NULL;
    CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &unpack);
    u8* buf = (u8*)malloc(kPiece + 0x10000);
    for (int i = 0; i < PORT_STACK_SLOTS; i++) {
        if (!h.active[i]) continue;
        if (!VirtualAlloc((void*)(uintptr_t)(PortStackTop(i) - (i == 0 ? 0x400000 : 0x100000)), i == 0 ? 0x400000 : 0x100000, MEM_COMMIT, PAGE_READWRITE))
            Refuse("cannot map the stack of thread %d", i);
    }
    u8* gfx = NULL;
    u32 gfxSize = 0, gfxBase = 0;
    BOOL testFolder;
    const char* saveRoot = PortNandRoot(&testFolder);
    if (testFolder) ClearSaveFiles(saveRoot);
    else fprintf(stderr, "[port] state: the save folder %s is left as it is (a state's save files are only put back into a MONSTERLAB_SAVE folder)\n", saveRoot);
    for (;;) {
        Chunk c;
        if (fread(&c, sizeof(c), 1, f) != 1) Refuse("the file is cut off");
        if (c.kind == CHUNK_END) break;
        if (c.kind == CHUNK_FILE) {
            if (c.size > kPiece) Refuse("damaged chunk");
            c.name[sizeof(c.name) - 1] = 0;
            if (fread(buf, 1, c.size, f) != c.size) Refuse("the file is cut off");
            if (testFolder && !strstr(c.name, "..") && !strchr(c.name, ':')) {
                char path[MAX_PATH + 300];
                snprintf(path, sizeof(path), "%s\\%s", saveRoot, c.name);
                FILE* out = fopen(path, "wb");
                if (out) { fwrite(buf, 1, c.size, out); fclose(out); }
            }
            continue;
        }
        u8* dst = (u8*)(uintptr_t)c.addr;
        if (c.kind == CHUNK_GFX) {                     // addresses are the saving process's: only the offsets count
            if (!gfx) { gfxSize = (u32)strtoul(c.name, NULL, 10); gfx = (u8*)calloc(gfxSize + 4, 1); gfxBase = c.addr; }
            if (!gfx || c.addr - gfxBase + c.size > gfxSize) Refuse("damaged chunk");
            dst = gfx + (c.addr - gfxBase);
        } else
        if (c.kind == CHUNK_SECTION) {
            u8* p; u32 n;
            c.name[sizeof(c.name) - 1] = 0;
            // Pieces of one section follow each other; the first names the start.
            if (!FindSection(c.name, &p, &n) || dst < p || dst + c.size > p + Align4(n)) Refuse("section %s of the exe has another size", c.name);
        } else if (c.kind == CHUNK_BLOB) {
            Blob* b = NULL;
            for (int i = 0; i < sBlobCount; i++) if (!strncmp(sBlobs[i].name, c.name, sizeof(c.name) - 1)) b = &sBlobs[i];
            if (!b || dst < (u8*)b->p || dst + c.size > (u8*)b->p + b->size) Refuse("block %s does not fit this exe", c.name);
        } else if (c.kind != CHUNK_MEM) Refuse("unknown chunk %u", c.kind);
        if (c.size > kPiece || c.packed > kPiece + 0x10000) Refuse("damaged chunk");
        if (!c.packed) { memset(dst, 0, c.size); continue; }
        SIZE_T got = 0;
        if (fread(buf, 1, c.packed, f) != c.packed || !unpack || !Decompress(unpack, buf, c.packed, dst, c.size, &got) || got != c.size)
            Refuse("damaged data (chunk %s at %08X)", c.name, c.addr);
    }
    fclose(f);
    free(buf);
    if (unpack) CloseDecompressor(unpack);
    if (gfx) PortGfxStateLoad(gfx, gfxSize);
    free(gfx);
    memcpy(sResume, h.ctx, sizeof(sResume));
    memcpy(sResumeActive, h.active, sizeof(sResumeActive));
    sLoaded = TRUE;
    fprintf(stderr, "[port] state: loaded frame %d from %s\n", h.frame, sLoadPath);
    PortSchedAfterLoad();
    for (s32 i = 0; i < sOnLoadCount; i++) sOnLoad[i]();
    PortOsAfterLoad();                                 // the other game threads: each waits for its turn, or goes on in Save()
    PortStackResume(0, &sResume[0]);                   // and so does the main thread
}

#endif  // _WIN32
