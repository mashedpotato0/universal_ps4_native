#!/usr/bin/env python3
# universal ps4 native graphical launcher
import os
import re
import subprocess
import sys
from pathlib import Path
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "scripts"
BIN = ROOT / "bin"
OUT = ROOT / "out"

# dark theme color palette with no purple
BG_DARK = "#121418"
PANEL_BG = "#1a1d24"
INPUT_BG = "#222630"
BORDER_COL = "#303644"
TEXT_MAIN = "#ebf0f5"
TEXT_MUTED = "#8c94a0"
ACCENT_BLUE = "#0070d1"
ACCENT_HOVER = "#0084f7"
BTN_SEC_BG = "#262a34"
BTN_SEC_HOVER = "#303542"


def discover_games():
    # scan candidate directories for extracted games or pkgs
    sys.path.insert(0, str(SCRIPTS))
    try:
        import pkg
        import prepare

        def read_sfo(game_dir: Path) -> dict:
            sfo = game_dir / "sce_sys/param.sfo"
            if not sfo.exists():
                return {}
            try:
                return prepare.sfo(sfo.read_bytes())
            except Exception:
                return {}

        candidates = [
            ROOT / "extracted",
            ROOT / "pkg",
            ROOT / "pkgs",
            ROOT.parent / "CUSA03173",
        ]
        for sibling in ROOT.parent.glob("CUSA*"):
            if sibling not in candidates:
                candidates.append(sibling)

        discovered = []
        for cand in candidates:
            if not cand.exists():
                continue
            if (cand / "eboot.bin").exists():
                sfo = read_sfo(cand)
                title_id = sfo.get("TITLE_ID", cand.name)
                title = sfo.get("TITLE", title_id)
                discovered.append({
                    "title": title,
                    "title_id": title_id,
                    "path": str(cand),
                    "type": "extracted",
                })
            elif cand.is_dir():
                for sub in sorted(cand.iterdir()):
                    if sub.is_dir() and (sub / "eboot.bin").exists():
                        sfo = read_sfo(sub)
                        title_id = sfo.get("TITLE_ID", sub.name)
                        title = sfo.get("TITLE", title_id)
                        discovered.append({
                            "title": title,
                            "title_id": title_id,
                            "path": str(sub),
                            "type": "extracted",
                        })
                for p in pkg.scan_pkgs(cand):
                    discovered.append({
                        "title": p["title"],
                        "title_id": p["title_id"],
                        "path": str(cand),
                        "type": "pkg",
                    })
        return discovered
    except Exception:
        return []


class LauncherApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Universal PS4 Native - Game Launcher")
        self.root.geometry("680x560")
        self.root.minsize(640, 520)
        self.root.configure(bg=BG_DARK)

        self.games = discover_games()
        self.setup_ui()

    def setup_ui(self):
        # configure ttk styles
        style = ttk.Style()
        style.theme_use("clam")

        style.configure("TCombobox",
                        fieldbackground=INPUT_BG,
                        background=PANEL_BG,
                        foreground=TEXT_MAIN,
                        darkcolor=BORDER_COL,
                        lightcolor=BORDER_COL,
                        arrowcolor=TEXT_MAIN,
                        bordercolor=BORDER_COL)

        main_frame = tk.Frame(self.root, bg=BG_DARK, padx=24, pady=20)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # header
        lbl_title = tk.Label(main_frame, text="UNIVERSAL PS4 NATIVE",
                             font=("Segoe UI", 15, "bold"), fg=TEXT_MAIN, bg=BG_DARK)
        lbl_title.pack(anchor="w")

        lbl_sub = tk.Label(main_frame, text="Direct PlayStation 4 Runtime & Execution Toolchain",
                           font=("Segoe UI", 9), fg=TEXT_MUTED, bg=BG_DARK)
        lbl_sub.pack(anchor="w", pady=(0, 14))

        # game selection card
        card_game = tk.LabelFrame(main_frame, text=" INSTALLED GAME / PACKAGE ",
                                  font=("Segoe UI", 9, "bold"), fg=TEXT_MAIN, bg=PANEL_BG,
                                  padx=14, pady=12, bd=1, relief=tk.SOLID)
        card_game.pack(fill=tk.X, pady=(0, 12))

        game_select_frame = tk.Frame(card_game, bg=PANEL_BG)
        game_select_frame.pack(fill=tk.X)

        self.combo_options = []
        for g in self.games:
            self.combo_options.append(f"{g['title']} ({g['title_id']}) [{g['type']}]")
        if not self.combo_options:
            self.combo_options = ["Bloodborne (CUSA03173)", "Sekiro: Shadows Die Twice (CUSA13801)", "Street Fighter (CUSA07997)"]
        self.combo_options.append("<Custom Path / Package...>")

        self.game_var = tk.StringVar(value=self.combo_options[0])
        self.combo_game = ttk.Combobox(game_select_frame, textvariable=self.game_var,
                                       values=self.combo_options, state="readonly", font=("Segoe UI", 9))
        self.combo_game.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 8))
        self.combo_game.bind("<<ComboboxSelected>>", self.on_game_selected)

        btn_browse = tk.Button(game_select_frame, text="Browse...", font=("Segoe UI", 9),
                               bg=BTN_SEC_BG, fg=TEXT_MAIN, activebackground=BTN_SEC_HOVER,
                               activeforeground=TEXT_MAIN, bd=0, padx=12, pady=4,
                               cursor="hand2", command=self.on_browse)
        btn_browse.pack(side=tk.RIGHT)

        self.path_var = tk.StringVar()
        if self.games:
            self.path_var.set(self.games[0]["path"])
        entry_path = tk.Entry(card_game, textvariable=self.path_var, font=("Segoe UI", 9),
                              bg=INPUT_BG, fg=TEXT_MAIN, insertbackground=TEXT_MAIN,
                              bd=1, relief=tk.SOLID)
        entry_path.pack(fill=tk.X, pady=(8, 0))

        # options card
        card_opts = tk.LabelFrame(main_frame, text=" RUNTIME & HARDWARE OPTIONS ",
                                  font=("Segoe UI", 9, "bold"), fg=TEXT_MAIN, bg=PANEL_BG,
                                  padx=14, pady=12, bd=1, relief=tk.SOLID)
        card_opts.pack(fill=tk.X, pady=(0, 14))

        opts_split = tk.Frame(card_opts, bg=PANEL_BG)
        opts_split.pack(fill=tk.X)

        # left checkboxes
        frame_checks = tk.Frame(opts_split, bg=PANEL_BG)
        frame_checks.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.low_spec_var = tk.BooleanVar(value=True)
        self.uncap_var = tk.BooleanVar(value=False)
        self.permissive_var = tk.BooleanVar(value=False)
        self.cpu_only_var = tk.BooleanVar(value=False)
        self.rebuild_var = tk.BooleanVar(value=False)

        def make_chk(parent, text, var):
            c = tk.Checkbutton(parent, text=text, variable=var, font=("Segoe UI", 9),
                               fg=TEXT_MAIN, bg=PANEL_BG, selectcolor=INPUT_BG,
                               activebackground=PANEL_BG, activeforeground=TEXT_MAIN,
                               bd=0, highlightthickness=0)
            c.pack(anchor="w", pady=2)
            return c

        make_chk(frame_checks, "Low-Spec / Laptop iGPU Profile", self.low_spec_var)
        make_chk(frame_checks, "Uncap Frame Rate (Display VSync)", self.uncap_var)
        make_chk(frame_checks, "Permissive Mode (Stub Unmapped OS)", self.permissive_var)
        make_chk(frame_checks, "Headless / CPU-Only Mode", self.cpu_only_var)
        make_chk(frame_checks, "Force Rebuild Native Binary", self.rebuild_var)

        # right dropdowns
        frame_dropdowns = tk.Frame(opts_split, bg=PANEL_BG)
        frame_dropdowns.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(16, 0))

        lbl_fps = tk.Label(frame_dropdowns, text="Target Frame Rate:",
                           font=("Segoe UI", 9), fg=TEXT_MUTED, bg=PANEL_BG)
        lbl_fps.pack(anchor="w")

        self.fps_var = tk.StringVar(value="Uncapped (Delta-Time)")
        combo_fps = ttk.Combobox(frame_dropdowns, textvariable=self.fps_var,
                                 values=["Uncapped (Delta-Time)", "60 FPS Cap", "30 FPS Cap", "90 FPS Cap"],
                                 state="readonly", font=("Segoe UI", 9), width=24)
        combo_fps.pack(anchor="w", pady=(2, 10))

        lbl_gpu = tk.Label(frame_dropdowns, text="Graphics Device:",
                           font=("Segoe UI", 9), fg=TEXT_MUTED, bg=PANEL_BG)
        lbl_gpu.pack(anchor="w")

        self.gpu_var = tk.StringVar(value="Auto-Detect (Best Available)")
        combo_gpu = ttk.Combobox(frame_dropdowns, textvariable=self.gpu_var,
                                 values=["Auto-Detect (Best Available)", "Force Discrete GPU", "Force Integrated GPU"],
                                 state="readonly", font=("Segoe UI", 9), width=24)
        combo_gpu.pack(anchor="w", pady=(2, 0))

        # primary launch button
        self.btn_launch = tk.Button(main_frame, text="LAUNCH GAME", font=("Segoe UI", 11, "bold"),
                                    bg=ACCENT_BLUE, fg="#ffffff", activebackground=ACCENT_HOVER,
                                    activeforeground="#ffffff", bd=0, pady=10, cursor="hand2",
                                    command=self.on_launch)
        self.btn_launch.pack(fill=tk.X, pady=(0, 10))

        # secondary button bar
        sec_frame = tk.Frame(main_frame, bg=BG_DARK)
        sec_frame.pack(fill=tk.X, pady=(0, 10))

        def make_sec_btn(text, cmd):
            b = tk.Button(sec_frame, text=text, font=("Segoe UI", 9),
                          bg=BTN_SEC_BG, fg=TEXT_MAIN, activebackground=BTN_SEC_HOVER,
                          activeforeground=TEXT_MAIN, bd=0, pady=6, cursor="hand2",
                          command=cmd)
            b.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=4)
            return b

        make_sec_btn("Inspect Game", self.on_inspect)
        make_sec_btn("Extract PKG", self.on_extract)
        make_sec_btn("Sync Launchers", self.on_sync)

        # status bar
        self.status_var = tk.StringVar(value="Ready - Select installed title or custom package to run.")
        lbl_status = tk.Label(main_frame, textvariable=self.status_var,
                              font=("Segoe UI", 8), fg=TEXT_MUTED, bg=BG_DARK, anchor="w")
        lbl_status.pack(fill=tk.X)

    def on_game_selected(self, event=None):
        idx = self.combo_game.current()
        if 0 <= idx < len(self.games):
            self.path_var.set(self.games[idx]["path"])

    def on_browse(self):
        filename = filedialog.askopenfilename(
            title="Select PS4 Package or eboot.bin",
            filetypes=[("PS4 Packages / Executables", "*.pkg;eboot.bin"), ("All Files", "*.*")]
        )
        if filename:
            self.path_var.set(filename)

    def build_flags(self):
        flags = []
        if self.uncap_var.get():
            flags.append("--no-cap-fps")
        fps_choice = self.fps_var.get()
        if "60" in fps_choice:
            flags.extend(["--fps", "60"])
        elif "30" in fps_choice:
            flags.extend(["--fps", "30"])
        elif "90" in fps_choice:
            flags.extend(["--fps", "90"])

        if self.low_spec_var.get():
            flags.append("--low-spec")
        if self.permissive_var.get():
            flags.append("--permissive")
        if self.cpu_only_var.get():
            flags.append("--cpu-only")
        if self.rebuild_var.get():
            flags.append("--rebuild")

        gpu_choice = self.gpu_var.get()
        if "Discrete" in gpu_choice:
            flags.append("--discrete")
        elif "Integrated" in gpu_choice:
            flags.append("--integrated")

        return flags

    def on_launch(self):
        target = self.path_var.get().strip()
        flags = self.build_flags()
        cmd = [sys.executable, str(BIN / "ps4-native"), "run"]
        if target:
            cmd.append(target)
        cmd.extend(flags)

        self.status_var.set(f"Launching game: {' '.join(cmd)}")
        self.root.update()
        try:
            subprocess.Popen(cmd, cwd=str(ROOT))
        except Exception as e:
            messagebox.showerror("Launch Error", f"Failed to execute launcher: {e}")

    def on_inspect(self):
        target = self.path_var.get().strip()
        if not target:
            messagebox.showwarning("Inspect", "Please select or browse a game folder or PKG file first.")
            return
        cmd = [sys.executable, str(BIN / "ps4-native"), "inspect", target]
        self.status_var.set(f"Inspecting: {target}...")
        try:
            res = subprocess.run(cmd, cwd=str(ROOT), capture_output=True, text=True)
            output = res.stdout if res.returncode == 0 else (res.stderr or res.stdout)
            messagebox.showinfo("Game Inspection", output[:1500] if output else "No metadata returned.")
        except Exception as e:
            messagebox.showerror("Inspect Error", str(e))

    def on_extract(self):
        target = self.path_var.get().strip()
        if not target or not target.lower().endswith(".pkg"):
            messagebox.showwarning("Extract", "Please select a valid .pkg file to extract.")
            return
        cmd = [sys.executable, str(BIN / "ps4-native"), "extract", target]
        self.status_var.set(f"Extracting package: {target}...")
        self.root.update()
        try:
            subprocess.Popen(cmd, cwd=str(ROOT))
            messagebox.showinfo("Extract", "Extraction started in background.")
        except Exception as e:
            messagebox.showerror("Extract Error", str(e))

    def on_sync(self):
        try:
            sys.path.insert(0, str(ROOT))
            import play
            created = play.sync_launchers()
            msg = f"Synchronized launchers: {', '.join(created)}" if created else "All launchers are up to date."
            self.status_var.set(msg)
            messagebox.showinfo("Sync Launchers", msg)
        except Exception as e:
            messagebox.showerror("Sync Error", str(e))


def main():
    root = tk.Tk()
    app = LauncherApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
