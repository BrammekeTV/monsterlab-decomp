"""Headless test of the Bink cutscene player (src/port/port_bink.cpp): the intro movie plays.

    python tools/movie_test.py [--iso PATH] [--keep]

Runs the exe unpaced with --wav to the end of the first movie, takes screenshots at a few frame counts inside it
and checks that the log says the movie was opened, that the pictures are not black, that they differ from each
other, and that the audio has sound while the movie runs. Output (PNGs, WAV) stays in build/movie_test/ (game
content: never commit it). Needs the game (orig/RM7P4F or --iso). Also runs once with --no-movies.
"""
import os, subprocess, sys, wave
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
EXE = os.path.join(ROOT, "build", "pc", "monsterlab.exe")
OUT = os.path.join(ROOT, "build", "movie_test")
FRAMES = (2500, 3000, 3500)       # inside the first movie (it starts at about frame 2330 and runs 29 s)

def png_stats(path):
    from PIL import Image
    a = np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)
    return a

def run(extra, tag, frames):
    png = os.path.join(OUT, f"{tag}_{frames}.png"); wav = os.path.join(OUT, f"{tag}_{frames}.wav")
    r = subprocess.run([EXE, "--fast", "--no-ini", "--no-mods", "--res", "640x480", "--seconds", "250", "--frames", str(frames),
                        "--screenshot", png, "--wav", wav] + extra, capture_output=True, text=True, timeout=400)
    return r.stdout + r.stderr, png, wav

def main():
    argv = sys.argv[1:]
    data = ["--iso", argv[argv.index("--iso") + 1]] if "--iso" in argv else []
    if not data:
        for root in (ROOT, r"F:\monsterlab_redecomp"):
            if os.path.isdir(os.path.join(root, "orig", "RM7P4F")): data = ["--data", os.path.join(root, "orig", "RM7P4F")]; break
    if not data: sys.exit("no game data (orig/RM7P4F or --iso)")
    os.makedirs(OUT, exist_ok=True)
    ok = True
    shots = []
    for n in FRAMES:
        log, png, wav = run(data, "movie", n)
        opened = "Bink: " in log and ".bik" in log
        stopped = "frame limit reached" in log
        a = png_stats(png); shots.append(a)
        lum = a.mean()
        print(f"frame {n}: opened={opened} reached_limit={stopped} mean luminance {lum:.1f}")
        ok &= opened and stopped and lum > 5
    ok &= any(np.abs(shots[0] - s).mean() > 2 for s in shots[1:])      # the picture moves
    w = wave.open(wav); d = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(float)
    loud = float(np.sqrt((d[-w.getframerate() * 2 * 20:] ** 2).mean()))   # the last 20 s of the run
    print(f"audio: {w.getnframes() / w.getframerate():.1f} s, rms of the last 20 s {loud:.0f}")
    ok &= loud > 200
    log, png, wav = run(data + ["--no-movies"], "nomovies", 3000)
    quiet = "Bink: " not in log and "frame limit reached" in log
    print(f"--no-movies: no movie opened and the run goes on: {quiet}")
    ok &= quiet
    print("MOVIE TEST OK" if ok else "MOVIE TEST FAIL")
    sys.exit(0 if ok else 1)
main()
