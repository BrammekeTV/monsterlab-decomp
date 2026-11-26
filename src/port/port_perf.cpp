// Frame-time log and fixed benchmark scenarios of the PC port (--perf-log FILE, --bench menu|village|battle|castle).
// One CSV row per shown frame, a summary at the end. No original code here.
#include "port/port.h"
#include "port/port_overlay.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

static FILE*             sCsv;
static const char*       sPath;
static std::vector<f32>  sTimes;        // wall ms of the frames that count (real time; all frames without --bench)
static s32               sBenchStart;   // real-time frames began at this presented frame (-1: not yet)
static f64               sSumGame, sSumRender, sSumWait, sSumPresent, sSumDraws, sSumVerts;
static f64               sSumDecode, sSumSetup, sSumFlush, sSumCopy;

void PortPerfOpen(const char* path) {
    sPath = path;
    sCsv = fopen(path, "w");
    if (!sCsv) { PortLog("perf: cannot write %s", path); return; }
    fprintf(sCsv, "frame,fast,wall_ms,game_ms,render_ms,wait_ms,present_ms,draws,vertices,copies,textures,out_w,out_h,decode_ms,setup_ms,flush_ms,copy_ms\n");
    sBenchStart = -1;
}

BOOL PortPerfActive(void) { return sCsv != NULL; }

// Called once per shown frame by the renderer (after the stats of that frame are known).
void PortPerfFrame(const PortFrameStats* s, f32 presentMs) {
    if (!sCsv) return;
    BOOL fast = PortFastActive();
    s32 frame = PortFrameCount();
    fprintf(sCsv, "%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.3f\n", frame, fast ? 1 : 0, s->frameMs, s->gameMs, s->renderMs, s->waitMs, presentMs,
            s->draws, s->verts, s->efbCopies, s->textures, s->outW, s->outH, s->decodeMs, s->setupMs, s->flushMs, s->copyMs);
    // The summary covers real-time frames only when the run fast-forwarded first (a benchmark); the first one has no frame time yet.
    if (fast || s->frameMs <= 0.0f) return;
    sTimes.push_back(s->frameMs);
    sSumGame += s->gameMs; sSumRender += s->renderMs; sSumWait += s->waitMs; sSumPresent += presentMs; sSumDraws += s->draws; sSumVerts += s->verts;
    sSumDecode += s->decodeMs; sSumSetup += s->setupMs; sSumFlush += s->flushMs; sSumCopy += s->copyMs;
}

void PortPerfFinish(void) {
    if (!sCsv) return;
    fclose(sCsv);
    sCsv = NULL;
    size_t n = sTimes.size();
    if (!n) { PortLog("perf: no real-time frames were shown (%s)", sPath); return; }
    std::vector<f32> v = sTimes;
    std::sort(v.begin(), v.end());
    f64 sum = 0;
    for (f32 t : v) sum += t;
    auto pct = [&](f64 p) { return v[std::min(n - 1, (size_t)(p * n))]; };
    f64 avg = sum / n;
    char sumPath[512];
    snprintf(sumPath, sizeof(sumPath), "%s.summary.txt", sPath);
    char text[1024];
    snprintf(text, sizeof(text),
             "frames %zu  average %.2f ms (%.1f fps)  p50 %.2f  p95 %.2f  p99 %.2f  worst %.2f ms\n"
             "per frame: game %.2f ms  render %.2f ms  wait %.2f ms  present %.2f ms  draws %.0f  vertices %.0f\n"
             "render split: decode %.2f ms  set-up %.2f ms  flush %.2f ms  copy %.2f ms\n",
             n, avg, 1000.0 / avg, pct(0.50), pct(0.95), pct(0.99), v.back(),
             sSumGame / n, sSumRender / n, sSumWait / n, sSumPresent / n, sSumDraws / n, sSumVerts / n,
             sSumDecode / n, sSumSetup / n, sSumFlush / n, sSumCopy / n);
    FILE* f = fopen(sumPath, "w");
    if (f) { fputs(text, f); fclose(f); }
    PortLog("perf: %s", text);
    PortLog("perf: rows in %s, summary in %s", sPath, sumPath);
}
