// In-process sampling profiler for the PC port (PORT_SAMPLE=1): every millisecond it reads the instruction
// pointer of the game thread (the one that runs the game and draws its pictures; the others only wait), counts the ones inside the executable by address and writes build/pc/samples.txt
// at exit (address, count; resolve with tools/sample_report.py and monsterlab.map). Threads waiting in the
// system (ntdll/kernel) count as "other". No original code here.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <atomic>

namespace {
std::map<unsigned, unsigned> sHist;
unsigned sOther, sDbg;
std::map<std::string, unsigned> sMods;
std::map<unsigned, unsigned> sCallers;      // callers of system code, by return address
std::atomic<bool> sStop;
std::atomic<DWORD> sTarget;                 // the thread sampled
HANDLE sThread;

void Write();
DWORD WINAPI SampleMain(void*) {
    HMODULE self = GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((char*)self + ((IMAGE_DOS_HEADER*)self)->e_lfanew);
    unsigned lo = (unsigned)(size_t)self, hi = lo + nt->OptionalHeader.SizeOfImage;
    unsigned n = 0;
    while (!sStop) {
        if (++n % 200 == 0) Write();   // the process may leave without running atexit handlers
        HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, sTarget.load());
        if (h) {
            // While the thread is suspended it may own the heap or loader lock: no allocation, no module lookup
            // here (that deadlocked the sampler); only copy the registers and scan the stack.
            unsigned ip = 0, caller = 0;
            bool have = false;
            if (SuspendThread(h) != (DWORD)-1) {
                CONTEXT c = {};
                c.ContextFlags = CONTEXT_CONTROL;
                if (GetThreadContext(h, &c)) {
                    ip = c.Eip; have = true;
                    if (!(ip >= lo && ip < hi)) {
                        // ReadProcessMemory fails quietly where a plain read would fault into the port's exception handler.
                        unsigned stack[400];
                        SIZE_T got = 0;
                        ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(size_t)c.Esp, stack, sizeof(stack), &got);
                        for (SIZE_T i = 0; i < got / 4; i++) if (stack[i] >= lo + 0x1000 && stack[i] < lo + 0x277000) { caller = stack[i] - lo; break; }
                    }
                }
                ResumeThread(h);
            }
            if (have) {
                if (ip >= lo && ip < hi) sHist[ip - lo]++;
                else {
                    sOther++;
                    if (caller) sCallers[caller]++;
                    HMODULE hm = NULL; char nm[MAX_PATH] = "?";
                    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(size_t)ip, &hm) && hm) GetModuleFileNameA(hm, nm, MAX_PATH);
                    const char* b2 = strrchr(nm, 92); sMods[b2 ? b2 + 1 : nm]++;
                }
            }
            CloseHandle(h);
        }
        Sleep(1);
    }
    return 0;
}

void Write() {
    FILE* f = fopen("build/pc/samples.txt", "w");
    if (!f) return;
    fprintf(f, "other %u\n", sOther);
    for (auto& m : sMods) fprintf(f, "# %s %u\n", m.first.c_str(), m.second);
    for (auto& e : sCallers) fprintf(f, "C%08X %u\n", e.first, e.second);
    for (auto& e : sHist) fprintf(f, "%08X %u\n", e.first, e.second);
    fclose(f);
}

}

// The thread to sample from now on (the renderer names the one that makes the pictures).
void PortSamplerThread(DWORD id) { sTarget = id; }

void PortSamplerStart() {
    const char* e = getenv("PORT_SAMPLE");
    if (!e || *e != '1' || sThread) return;
    sTarget = GetCurrentThreadId();         // the caller goes on to run the game
    sThread = CreateThread(NULL, 0, SampleMain, NULL, 0, NULL);
}
