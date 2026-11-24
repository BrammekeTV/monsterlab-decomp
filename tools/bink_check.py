"""Compare the PC Bink decoder (src/port/port_bink_codec.cpp) with FFmpeg on a movie from your disc.

    python tools/bink_check.py [file.bik] [--ffmpeg PATH]

Builds tests/bink_test.cpp, decodes a few frames and the audio of track 0, decodes the same with ffmpeg
(needs ffmpeg.exe on PATH; used only as a reference, not shipped) and compares. Output stays in build/bink_test/.
"""
import os, subprocess, sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import check

def main():
    argv = sys.argv[1:]
    ff = "ffmpeg"
    if "--ffmpeg" in argv:
        i = argv.index("--ffmpeg"); ff = argv[i + 1]; del argv[i:i + 2]
    cands = [os.path.join(r, "orig", "RM7P4F", "files", "fmv_PAL", "Alien_Intro.bik") for r in (ROOT, r"F:\monsterlab_redecomp")]
    path = argv[0] if argv else next((c for c in cands if os.path.exists(c)), None)
    if not path: sys.exit("no .bik found: pass one")
    out = os.path.join(ROOT, "build", "bink_test"); os.makedirs(out, exist_ok=True)
    exe = os.path.join(out, "bink_test.exe")
    flags = [f for f in check.FLAGS if f != "/c"]
    srcs = [os.path.join(ROOT, "tests", "bink_test.cpp"), os.path.join(ROOT, "src", "port", "port_bink_codec.cpp")]
    script = os.path.join(out, "build.bat")
    with open(script, "w") as f:
        f.write('@call "' + check.vcvars() + '" >nul\r\n')
        f.write("@cl " + " ".join(flags) + ' /O2 /Fo"' + out + '\\\\" /Fe"' + exe + '" ' + " ".join('"' + s + '"' for s in srcs) + "\r\n")
    r = subprocess.run(["cmd", "/c", script], capture_output=True, text=True)
    if r.returncode != 0: print(r.stdout[-4000:]); sys.exit("build failed")
    frames = [0, 1, 30, 150]
    r = subprocess.run([exe, path, "0", out] + [str(x) for x in frames], capture_output=True, text=True)
    print(r.stdout.strip())
    if r.returncode != 0: sys.exit("decoder failed")
    ok = True
    w, h = [int(x) for x in r.stdout.split()[2].split("x")]
    for n in frames:
        ref = os.path.join(out, f"ref_{n}.yuv")
        subprocess.run([ff, "-v", "error", "-y", "-i", path, "-vf", f"select=eq(n\,{n})", "-frames:v", "1", "-vsync", "0",
                        "-pix_fmt", "yuv420p", "-f", "rawvideo", ref], check=True)
        a = np.fromfile(os.path.join(out, f"frame_{n}.yuv"), np.uint8); b = np.fromfile(ref, np.uint8)
        same = a.size == b.size and (a == b).all()
        print(f"frame {n}: {'identical' if same else 'DIFFERENT max diff ' + str(int(np.abs(a.astype(int) - b.astype(int)).max())) if a.size == b.size else 'size ' + str((a.size, b.size))}")
        ok &= same
    ref = os.path.join(out, "ref_audio.f32")
    subprocess.run([ff, "-v", "error", "-y", "-i", path, "-map", "0:a:0", "-t", "10", "-f", "f32le", ref], check=True)
    a = np.fromfile(os.path.join(out, "audio.f32"), np.float32); b = np.fromfile(ref, np.float32)
    n = min(a.size, b.size)
    err = np.abs(a[:n] - b[:n]).max() if n else 1e9
    print(f"audio: {a.size} vs {b.size} samples, max abs error {err:.3g} (peak {np.abs(b).max():.3g})")
    ok &= n > 0 and err < 1e-3
    print("BINK CHECK OK" if ok else "BINK CHECK FAIL")
    sys.exit(0 if ok else 1)
main()
