"""Headless playthrough test of the PC port: run it before opening a PR that changes gameplay.

    python tools/playthrough.py [--iso PATH] [--no-build] [--stop-after STAGE] [--seconds N] [--retries N] [--gpu]
    python tools/playthrough.py --list
    python tools/playthrough.py --check-log FILE          (judge a saved log, run nothing)

Builds incrementally (tools/build_pc.py), fails at once if a game function is only a link stub,
then plays one continuous, unpaced run (no window, no sound, no movies, fixed random seed, empty
save folder) from boot through both village battles to the return to the castle, with scripted --press and
--pointer input. A stage passes when its game-state markers show up in the log (script calls,
level loads and PORT_STATE_LOG lines, never pixels) before the stage's deadline frame. The game
is stopped at the first missed deadline, crash or stall, and as soon as the last marker is seen.
A stall (hang) fails the test; --retries N repeats a stalled run and names it in the verdict.
Prints one line per stage and a verdict; exits 0/1. The log is build/playthrough/run.log.
Needs the game in orig/RM7P4F, MONSTERLAB_DATA or --iso PATH.

Adding a later stage: append a Stage to STAGES with the input it needs (absolute frame numbers)
and one or more markers. See docs/playthrough.md.
"""
import argparse, os, queue, re, shutil, subprocess, sys, threading, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PC = os.path.join(ROOT, "build", "pc")
OUT = os.path.join(ROOT, "build", "playthrough")      # the exe copy, its empty save folder and the log
# Its own name: a `taskkill /IM monsterlab.exe` meant for another run must not end this one.
EXE = os.path.join(OUT, "ml_playthrough.exe")
LOG = os.path.join(OUT, "run.log")
WIDTH, HEIGHT = 640, 360                              # small: headless runs draw in software (WARP)
STALL = 90                                            # wall seconds without a new frame number

FRAME = re.compile(r"^\[script\] .*frame (\d+)\)|^\[state\] frame (\d+):")


# A marker is (description, pattern, earliest frame). It is searched in every log line; its frame is the
# latest frame number the log has shown.
def call(name, after=0):
    """The script engine calls the function `name` at frame `after` or later."""
    return (f"call {name}", re.compile(r"^\[script\] " + name + r"\+0x"), after)


def level(name, after=0):
    """The level file `name`.sir is loaded and converted."""
    return (f"load {name}.sir", re.compile(r"^\[port\] " + name + r"\.sir header converted"), after)


def state(what, pattern, after=0):
    """A [state] line (PORT_STATE_LOG) matching `pattern`."""
    return (what, re.compile(r"^\[state\] frame \d+: " + pattern), after)


class Stage:
    def __init__(self, key, what, deadline, markers, press=(), pointer=()):
        self.key, self.what, self.deadline = key, what, deadline
        self.markers = markers                  # all must be seen (in any order; one log line counts once)
        self.press, self.pointer = press, pointer


# Frames are PortFrameDone counts. The timeline repeats (fixed seed); the deadlines leave a few hundred
# frames for small changes. Pointer positions are given in pixels of
# a 1920x1080 picture and scaled to the size the game renders at (WIDTH x HEIGHT).
# A on the map is "interact": no stray A presses there.
STAGES = [
    Stage("boot", "boot to the splash screens", 2300,
          [call("ShowSplashScreen")]),
    Stage("menu", "title, A, main menu", 3500,                      # A@2600 is --to-menu's own
          [level("castle_sea"), call("UIScreen_Push", 2900)],
          press=("A@3000-3006",)),
    Stage("join", "Join Now loads the Command Room", 3900,
          [call("StartQuest", 3500), level("castle_communication", 3500)],
          press=("A@3600-3610",), pointer=("663,200@3500",)),
    Stage("intro", "village intro: briefing, village, coffin lands", 5100,
          [call("StartConversationID", 3700), call("StopConversation", 4000),
           level("village_level", 4000), call("SetCoffinLanded", 4300)],
          press=("A@4000-4006", "A@4200-4206")),
    Stage("mayor", "walk to the Mayor and hear them out", 6900,     # Fuseless's "don't return" first
          [call("StopConversation", 5400), call("StartConversationID", 5650), call("StopConversation", 6500)],
          press=("A@5300-5306", "A@5500-5506", "LEFT@5650-5700",
                 "A@6000-6006", "A@6200-6206", "A@6400-6406", "A@6600-6606")),
    Stage("battle", "meet the enemy, arena and combat HUD up", 8100,
          [level("village_arena", 7400), call("UpdateCombatHUD", 7400),
           state("battle round 1", r"battle phase \d+ round 1 ", 7400)],
          press=("UP@7250-7300", "UP@7450-7500")),
    Stage("attack", "tutorial lines, first attack chosen", 11500,
          [call("SetActionSelected", 10000)],
          press=("A@8500-9250/150", "STICK_UP@9450-13300", "A@9600-10400/200", "A@10600"),
          pointer=("1380,720@10500",)),
    Stage("won", "battle won, Victory screen", 16000,
          [state("battle phase 8 (enemy defeated)", r"battle phase 8 ", 11000),
           call("SetUpCombatResultsScreen", 11000)],
          press=("A@10900-13300/150", "A@13500-14250/150", "A@14400-16000/150",
                 "STICK_UP@14300-15200", "STICK_UP@15500-16400")),
    Stage("rewards", "results continue, rewards screen", 16900,
          [call("SetUpCombatRewardsScreen", 16000)],
          pointer=("1430,940@16400",)),
    Stage("repair", "back in the village, Field Repair instructions, minigame starts", 18300,
          [level("village_level", 16500), call("StartChallenge", 16500), call("LoadChallengeUI", 17400)],
          press=("A@16500-17300/150", "A@17500", "A@17800"),
          pointer=("1400,790@17400", "1395,935@17700")),
    Stage("repaired", "Field Repair played to time-up, health handed back", 19300,
          [call("DeinitChallenge", 18500), call("AssignFieldRepairHealthPointsToPlayer", 18500)],
          press=("STICK_UP@18000-18300", "SWING@18300-19000")),
    Stage("freemap", "free on the village map: conversation closed, a walk works", 19900,
          [call("StopConversation", 19100),
           state("exploration HUD", r"hud machine 090B84CE \(depth 1\) state 0 ", 19100),
           call("target_node_changed", 19500), call("character_exploration_idle_entry", 19600),
           state("on the node right of the battle node", r"map node -1238753613 ", 19500)],
          press=("A@19150-19320/150", "RIGHT@19500-19550")),
    Stage("battle2", "walk up into the second monster, arena and combat HUD up", 20600,
          [call("CombatPlaceholderEntry", 19800), level("village_arena", 19800), call("UpdateCombatHUD", 19800),
           state("battle round 1", r"battle phase \d+ round 1 ", 19900)],
          press=("UP@19800-19850",)),
    # The second fight's length differs between runs (2300 to 4200 frames seen), so its input goes on until 26500:
    # A presses on the Victory screen with the pointer off its arrow do nothing. The screens after it start at 26700.
    Stage("won2", "second battle won, Victory screen", 26600,
          [state("battle phase 8 (enemy defeated)", r"battle phase 8 ", 20600),
           call("SetUpCombatResultsScreen", 20600)],
          press=("A@20550-26500/150", "STICK_UP@20450-21350", "STICK_UP@21650-22550", "STICK_UP@22850-23750",
                 "STICK_UP@24050-24950", "STICK_UP@25250-26150"),
          pointer=("1380,720@20400",)),
    Stage("awards", "results continue, Combat Awards, back on the map with the next quest step", 27800,
          [call("SetUpCombatRewardsScreen", 26700), level("village_level", 26900),
           call("SpawnDatabaseMonsterAtNode", 26900),
           state("exploration HUD", r"hud machine 090B84CE \(depth 1\) state 0 ", 26900)],
          press=("A@26800", "A@27150"),                                 # the awards' arrow is not where the results' is
          pointer=("1430,940@26700", "1350,855@27050")),
    Stage("thanks", "walk to the Mayor, his thanks, the game returns to the castle", 29400,
          [state("on the Mayor's node", r"map node -369066522 ", 27500),
           call("StartConversationID", 28100), call("GotoCastleLocation", 28100),
           level("castle_communication", 28100), call("HandleReturnToCastle", 28100), call("StartQuest", 28100)],
          press=("LEFT@27500-27550", "A@28100", "A@28250-28700/150")),
    # In the castle Fuseless talks; his last line needs A. The pointer leaves the place of the name entry's confirm
    # button first, so the A presses that come after the screen opened do not confirm an empty name.
    Stage("name", "Fuseless in the castle, the scientist name entry screen opens", 30200,
          [call("EnterName_Init", 28500), call("MPNameInput", 28500),
           state("name entry HUD state", r"hud machine 96773521 \(depth 1\) state 20 ", 28500)],
          press=("A@29000-29900/150",), pointer=("960,1040@28900",)),
    # The letter keys are the cells of a grid widget: point at one, A types it. Then the green tick confirms.
    Stage("named", "type a name on the letter keys, confirm, Fuseless's welcome", 31300,
          [call("EnterName_AddLetter", 30300), call("EnterName_SaveNameToPlayerName", 30700),
           call("EnterName_Destroy", 30700), call("StartConversationID", 30800)],
          press=("A@30400", "A@30600", "A@30850"),
          pointer=("445,325@30300", "520,325@30500", "1370,880@30750")),
    # Later stages go here (a deadline after 31300, frames counted from the run's start).
]


class Judge:
    """Reads log lines and tracks the stages. `failed` holds the reason once the run is lost."""
    def __init__(self, stages):
        self.stages, self.stage, self.frame = stages, 0, 0
        self.missing = list(stages[0].markers)  # the current stage's markers not seen yet
        self.done = []                          # frame at which each passed stage was complete
        self.failed = None
        self.tail = []                          # the last lines that are not script trace

    @property
    def finished(self):
        return self.stage == len(self.stages)

    def line(self, s):
        m = FRAME.match(s)
        if m:
            self.frame = max(self.frame, int(m.group(1) or m.group(2)))
        if not s.startswith("[script]"):
            self.tail = (self.tail + [s.rstrip()[:200]])[-12:]
        if self.failed or self.finished:
            return
        if "crash: exception" in s:
            self.failed = f"crash at frame {self.frame}"
            return
        if s.startswith("[port] STOP:") and "frame limit reached" not in s:
            self.failed = f"game stopped at frame {self.frame}: {s[len('[port] STOP: '):].strip()[:300]}"
            return
        for i, (what, pattern, after) in enumerate(self.missing):
            if self.frame >= after and pattern.search(s):
                del self.missing[i]
                break
        if not self.missing:
            self.done.append(self.frame)
            self.stage += 1
            if not self.finished:
                self.missing = list(self.stages[self.stage].markers)
        if not self.finished and self.frame > self.stages[self.stage].deadline:
            self.failed = f"not reached by frame {self.stages[self.stage].deadline}"

    def report(self, why, seconds, note=""):
        for i, st in enumerate(self.stages):
            if i < len(self.done):
                print(f"  PASS  {st.key:9} frame {self.done[i]:>6}  {st.what}")
            elif i == len(self.done):
                print(f"  FAIL  {st.key:9} {'':12}  {st.what}")
            else:
                print(f"  ----  {st.key:9} {'':12}  {st.what}")
        timing = f" ({seconds:.0f}s)" if seconds is not None else ""
        if self.finished:
            print(f"PLAYTHROUGH OK{timing}: {len(self.done)}/{len(self.stages)} stages, frame {self.frame}{note}")
            return 0
        st = self.stages[self.stage]
        waiting = "', '".join(m[0] for m in self.missing)
        reached = self.stages[self.stage - 1].key if self.stage else "nothing"
        print(f"PLAYTHROUGH FAIL{timing}: stage '{st.key}': {why}; waiting for '{waiting}'; "
              f"last passed: {reached}; log ends at frame {self.frame}{note}")
        for s in self.tail:
            print("    | " + s)
        return 1


def stub_count():
    """Game functions that were linked as stubs (an undefined function, e.g. after a rename)."""
    names, inside = [], False
    with open(os.path.join(PC, "port_link.txt"), errors="replace") as f:
        for s in f:
            if s.startswith("Game function stubs:"):
                inside = True
            elif inside:
                if not s.startswith("  0x"):
                    break
                names.append(s.strip())
    return names


def build():
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "build_pc.py")], cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        print("PLAYTHROUGH FAIL: build failed\n" + (r.stdout + r.stderr)[-2000:])
        return False
    return True


def run(stages, iso, seconds, gpu=False):
    judge = Judge(stages)
    os.makedirs(OUT, exist_ok=True)
    shutil.rmtree(os.path.join(OUT, "save"), ignore_errors=True)    # a save would add "Continue" and move the menu
    shutil.copy2(os.path.join(PC, "monsterlab.exe"), EXE)
    shutil.copy2(os.path.join(PC, "monsterlab.map"), os.path.splitext(EXE)[0] + ".map")   # names in crash reports
    cmd = [EXE, "--to-menu", "--fast", "--no-audio", "--no-movies", "--no-ini", "--no-mods",
           "--res", f"{WIDTH}x{HEIGHT}", "--frames", str(stages[-1].deadline + 1), "--seconds", str(seconds)]
    for st in stages:
        for p in st.press:
            cmd += ["--press", p]
        for p in st.pointer:
            xy, at = p.split("@")
            x, y = (int(v) for v in xy.split(","))
            cmd += ["--pointer", f"{round(x * WIDTH / 1920)},{round(y * HEIGHT / 1080)}@{at}"]
    if iso:
        cmd += ["--iso", iso]
    if gpu:
        cmd += ["--gpu"]
    env = {k: v for k, v in os.environ.items() if not k.startswith("PORT_")}
    env.update(PORT_SCRIPT_TRACE="1", PORT_STATE_LOG="1", MONSTERLAB_SAVE=os.path.join(OUT, "save"))
    proc = subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, errors="replace")
    lines = queue.Queue(maxsize=10000)

    def pump():
        for s in proc.stdout:
            lines.put(s)
        lines.put(None)

    threading.Thread(target=pump, daemon=True).start()
    why, frame, moved, grace = None, 0, time.time(), None
    with open(LOG, "w", encoding="utf-8", errors="replace") as log:
        while True:
            try:
                s = lines.get(timeout=1)
            except queue.Empty:
                s = ""
            if s is None:
                break
            if s:
                log.write(s)
                judge.line(s)
            now = time.time()
            if judge.frame != frame:
                frame, moved = judge.frame, now
            if judge.finished:
                break
            if judge.failed:
                # a crash report or STOP follows over the next lines: read on briefly
                grace = grace or now + (3 if judge.failed.startswith(("crash", "game stopped")) else 0)
                if now >= grace:
                    why = judge.failed
                    break
            elif now - moved > STALL:
                why = f"stalled: no new frame for {STALL}s (the game hangs or loops)"
                break
    if proc.poll() is None and (judge.finished or why):
        proc.kill()
    code = proc.wait()
    if not judge.finished and why is None and code == 0:
        why = judge.failed or "markers not seen before the frame limit"
    elif not judge.finished and why is None:
        why = judge.failed or f"the game ended early with exit code {code} (no crash report: killed, or MSVC's stack check)"
    return judge, why


def main():
    ap = argparse.ArgumentParser(description="headless playthrough test of the PC port")
    ap.add_argument("--iso", help="disc image or extracted disc (default: orig/RM7P4F or MONSTERLAB_DATA)")
    ap.add_argument("--no-build", action="store_true", help="use the exe in build/pc as it is")
    ap.add_argument("--stop-after", metavar="STAGE", help="end the test after this stage")
    ap.add_argument("--seconds", type=int, default=1500, help="wall-clock limit of the game run (default 1500)")
    ap.add_argument("--retries", type=int, default=0, help="runs to repeat after a stall (default 0; crashes are never retried)")
    ap.add_argument("--gpu", action="store_true", help="draw on the graphics card; only for problems that need it (parallel runs can crash the driver)")
    ap.add_argument("--list", action="store_true", help="list the stages")
    ap.add_argument("--check-log", metavar="FILE", help="judge a saved log instead of running the game")
    a = ap.parse_args()
    stages = STAGES
    if a.stop_after:
        keys = [s.key for s in STAGES]
        if a.stop_after not in keys:
            ap.error(f"unknown stage {a.stop_after}; stages: {', '.join(keys)}")
        stages = STAGES[:keys.index(a.stop_after) + 1]
    if a.list:
        for s in stages:
            print(f"{s.key:9} by frame {s.deadline:>6}  {s.what}  [{'; '.join(m[0] for m in s.markers)}]")
        return 0
    if a.check_log:
        judge = Judge(stages)
        with open(a.check_log, encoding="utf-8", errors="replace") as f:
            for s in f:
                judge.line(s)
        return judge.report(judge.failed or "marker not in the log", None)
    t0 = time.time()
    if not a.no_build and not build():
        return 1
    stubs = stub_count()
    if stubs:
        print(f"PLAYTHROUGH FAIL: {len(stubs)} game function(s) linked as stubs (build/pc/port_link.txt):\n    "
              + "\n    ".join(stubs[:15]))
        return 1
    judge, why = run(stages, a.iso, a.seconds, a.gpu)
    note = ""
    for n in range(a.retries):
        if not (why and why.startswith("stalled")):
            break
        kept = os.path.join(OUT, "stall.log")
        shutil.copy2(LOG, kept)
        note = f"; NOTE: the first run hung at frame {judge.frame} ({os.path.relpath(kept, ROOT)}), this is a retry"
        print(f"  {why} at frame {judge.frame}; retrying", flush=True)
        judge, why = run(stages, a.iso, a.seconds, a.gpu)
    code = judge.report(why, time.time() - t0, note)
    if code:
        print(f"    full log: {os.path.relpath(LOG, ROOT)}")
    return code


if __name__ == "__main__":
    sys.exit(main())
