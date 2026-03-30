#!/usr/bin/env python3
import os
import sys
import tkinter as tk
from tkinter import messagebox, colorchooser
import math
import subprocess
import threading
import re

EXECUTABLE = os.path.join(os.path.dirname(__file__), "..", "cmake-build-release", "hexodus", )
if not os.path.exists(EXECUTABLE):
    EXECUTABLE = os.path.join(os.path.dirname(__file__), "..", "cmake-build-debug", "hexodus", )


class HexGameUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Hexodus GUI")
        # Increase default window size
        self.root.geometry("1024x768")

        self.colors = {"Player 1 (White)": "#FFC107",
                       "Player 2 (Black)": "#0D6EFD"}  # More vibrant yellow/amber and vivid blue
        self.pieces = {}  # (q, r): color
        self.hex_size = 25
        self.offset_x = 512
        self.offset_y = 384
        self.current_turn_moves = []
        self.history = []  # list of (color, list_of_coords) to handle undo
        self.redo_stack = []  # allow redo
        self.player_turn = 0  # 0 for white (1 piece played), 1 for black (2 pieces) etc
        self.bot_playing = False
        self.evaluation = 0.0
        self.hovered_hex = None

        self.pan_start = None
        self.is_panning = False  # Track if user is panning to prevent accidental clicks

        self.setup_ui()
        self.start_engine()
        self.reset_game_state()
        self.root.bind('<Configure>', self.on_resize)
        self.root.update_idletasks()  # Ensure dimensions are calculated correctly initially
        self.draw_board()

    def start_engine(self):
        try:
            self.engine = subprocess.Popen([EXECUTABLE], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE, text=True, bufsize=1)
            threading.Thread(target=self.engine_reader, daemon=True).start()
        except FileNotFoundError:
            messagebox.showerror("Error", f"Could not find executable at {EXECUTABLE}. Please build it.")
            sys.exit(1)

    def write_engine(self, cmd):
        self.engine.stdin.write(cmd + "\n")
        self.engine.stdin.flush()
        if hasattr(self, 'root'):
            self.root.after(0, self.append_log, f"> {cmd}")

    def append_log(self, text):
        if hasattr(self, 'terminal') and self.terminal:
            self.terminal.config(state=tk.NORMAL)
            self.terminal.insert(tk.END, text + "\n")
            self.terminal.see(tk.END)
            self.terminal.config(state=tk.DISABLED)

    def engine_reader(self):
        while line := self.engine.stdout.readline():
            line = line.strip()
            self.root.after(0, self.append_log, line)
            if line.startswith("Evaluation:"):
                try:
                    self.evaluation = float(line.split(":")[1].strip())
                    if self.history:
                        h = self.history[-1]
                        self.history[-1] = (h[0], h[1], self.evaluation, h[3])
                    self.root.after(0, self.update_status)
                except ValueError:
                    pass
            elif line.startswith("Best move:"):
                if m := re.search(r"Best move: \((-?\d+), (-?\d+)\) and \((-?\d+), (-?\d+)\)", line):
                    res = (int(m.group(1)), int(m.group(2))), (int(m.group(3)), int(m.group(4)))
                    if getattr(self, "waiting_for_bot", False):
                        self.waiting_for_bot = False
                        self.root.after(0, lambda r=res: self.apply_bot_move(r))
                    elif getattr(self, "waiting_for_analysis", False):
                        self.waiting_for_analysis = False
                        self.suggestion = res
                        if self.history:
                            h = self.history[-1]
                            self.history[-1] = (h[0], h[1], h[2], self.suggestion)
                        self.root.after(0, self.draw_board)

    def setup_ui(self):
        self.root.configure(bg="#2E3440")

        self.board_frame = tk.Frame(self.root, bg="#2E3440")
        self.board_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(20, 10), pady=20)

        self.canvas = tk.Canvas(self.board_frame, bg="#3B4252", highlightthickness=0,
                                bd=0)  # Slightly darker canvas background for better contrast with tiles
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Button-1>", self.on_click_start)
        self.canvas.bind("<B1-Motion>", self.on_pan)
        self.canvas.bind("<ButtonRelease-1>", self.on_click_release)
        self.canvas.bind("<MouseWheel>", self.on_zoom)
        self.canvas.bind("<Button-4>", self.on_zoom)
        self.canvas.bind("<Button-5>", self.on_zoom)
        self.canvas.bind("<Motion>", self.on_mouse_move)
        self.canvas.bind("<Leave>", self.on_mouse_leave)

        # Control panel styles
        ctrl_bg, fg_col, btn_bg, btn_fg = "#2E3440", "#D8DEE9", "#434C5E", "#ECEFF4"

        control_frame = tk.Frame(self.root, width=320, bg=ctrl_bg, padx=20, pady=20)
        control_frame.pack(side=tk.RIGHT, fill=tk.Y)
        control_frame.pack_propagate(False)

        tk.Label(control_frame, text="Hexodus", font=("Helvetica", 24, "bold"), bg=ctrl_bg, fg="#88C0D0").pack(
            pady=(0, 25))

        self.status_var, self.eval_var = tk.StringVar(), tk.StringVar(value="Eval: 0.00")

        # Make the status text dynamically fit nicely
        self.status_label = tk.Label(control_frame, textvariable=self.status_var, font=("Helvetica", 12), bg=ctrl_bg,
                                     fg=fg_col, justify=tk.LEFT, anchor="w", wraplength=240)
        self.status_label.pack(fill=tk.X, pady=5)

        tk.Label(control_frame, textvariable=self.eval_var, font=("Consolas", 14, "bold"), bg=ctrl_bg, fg="#A3BE8C",
                 justify=tk.LEFT, anchor="w").pack(fill=tk.X, pady=(0, 20))

        def make_btn(text, cmd, bg_col=btn_bg):
            btn = tk.Button(control_frame, text=text, command=cmd, bg=bg_col, fg=btn_fg, font=("Helvetica", 11), bd=0,
                            relief="flat", activebackground="#4C566A", activeforeground="#FFFFFF", pady=8,
                            cursor="hand2")
            btn.pack(fill=tk.X, pady=6)
            return btn

        def make_header(text):
            tk.Label(control_frame, text=text, font=("Helvetica", 10, "bold"), bg=ctrl_bg, fg="#4C566A",
                     anchor="w").pack(fill=tk.X, pady=(15, 5))

        make_header("GAME")

        button_row = tk.Frame(control_frame, bg=ctrl_bg)
        button_row.pack(fill=tk.X, pady=2)
        tk.Button(button_row, text="Undo", command=self.on_undo, bg=btn_bg, fg=btn_fg, font=("Helvetica", 11), bd=0,
                  relief="flat", activebackground="#4C566A", activeforeground="#FFFFFF", pady=8, cursor="hand2").pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 2))
        tk.Button(button_row, text="Redo", command=self.on_redo, bg=btn_bg, fg=btn_fg, font=("Helvetica", 11), bd=0,
                  relief="flat", activebackground="#4C566A", activeforeground="#FFFFFF", pady=8, cursor="hand2").pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(2, 0))

        make_btn("Reset Game", self.on_reset)

        make_header("ENGINE")
        make_btn("Bot Move", self.on_bot_move)
        make_btn("Analyse (Suggest)", self.on_analyse)
        self.bot_vs_bot_btn = make_btn("Bot vs Bot: OFF", self.toggle_bot_vs_bot, "#dc3545")
        self.bot_vs_bot_btn.config(activebackground="#e4606d")

        make_header("APPEARANCE")
        make_btn("P1 Color", lambda: self.change_color("Player 1 (White)"))
        make_btn("P2 Color", lambda: self.change_color("Player 2 (Black)"))

        make_header("TERMINAL")
        term_frame = tk.Frame(control_frame, bg=ctrl_bg)
        term_frame.pack(fill=tk.BOTH, expand=True, pady=(5, 0))
        self.terminal = tk.Text(term_frame, bg="#3B4252", fg="#ECEFF4", font=("Consolas", 9), bd=0, relief="flat",
                                state=tk.DISABLED)
        scroll = tk.Scrollbar(term_frame, command=self.terminal.yview)
        self.terminal.config(yscrollcommand=scroll.set)
        self.terminal.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)

    def on_resize(self, event):
        if event.widget == self.canvas:
            self.draw_board()

    def reset_game_state(self):
        self.write_engine("reset")
        self.pieces.clear()
        self.pieces[(0, 0)] = self.colors["Player 1 (White)"]
        self.history = [("Player 1 (White)", [(0, 0)], 0.0, None)]
        self.redo_stack = []
        self.current_turn_moves = []
        self.player_turn = 1
        self.suggestion = None
        self.bot_playing = False
        self.game_over = False
        self.evaluation = 0.0
        self.hovered_hex = None
        self.bot_vs_bot_btn.config(text="Bot vs Bot: OFF", bg="#dc3545", activebackground="#e4606d")
        self.update_status()

    def update_status(self):
        if self.game_over: return
        player = "Player 1 (White)" if self.player_turn == 0 else "Player 2 (Black)"
        text = f"Turn: {player}\nPieces: {len(self.current_turn_moves)}/2"
        if self.suggestion:
            text += f"\nSuggest: {self.suggestion[0]} {self.suggestion[1]}"  # Keep cleanly mapped on one line
        self.status_var.set(text)
        sign = "+" if self.evaluation > 0 else ""
        self.eval_var.set(f"Eval: {sign}{self.evaluation:.2f}")

    def change_color(self, player):
        if color_code := colorchooser.askcolor(title=f"Choose color for {player}")[1]:
            self.colors[player] = color_code
            self.pieces = {c: self.colors[item[0]] for item in self.history for c in item[1]}
            self.draw_board()

    def draw_board(self):
        self.canvas.delete("all")
        cw, ch = self.canvas.winfo_width(), self.canvas.winfo_height()

        # Draw grid
        r_min, r_max = int(-self.offset_y / (self.hex_size * 1.5)) - 1, int(
            (ch - self.offset_y) / (self.hex_size * 1.5)) + 1
        for r in range(r_min, r_max + 1):
            x_offset = self.offset_x + self.hex_size * math.sqrt(3) * (r / 2.0)
            q_min, q_max = int(-x_offset / (self.hex_size * math.sqrt(3))) - 1, int(
                (cw - x_offset) / (self.hex_size * math.sqrt(3))) + 1
            for q in range(max(-64, q_min), min(64, q_max) + 1):
                if (q, r) not in self.pieces: self.draw_hex(q, r, outline="#4C566A")

        self.draw_hex(0, 0, outline="#EBCB8B", width=2)  # Center highlighted
        for (q, r), c in self.pieces.items(): self.draw_hex(q, r, fill=c, outline="#2E3440", width=2)
        
        # Color the currently pending player move more visibly matching player color theme with opacity visual feel via lighter saturation
        current_color = self.colors["Player 1 (White)"] if self.player_turn == 0 else self.colors["Player 2 (Black)"]
        for (q, r) in self.current_turn_moves: 
            self.draw_hex(q, r, fill=current_color, outline="#ECEFF4", width=3, stipple="gray50")

        if self.suggestion:
            for (q, r) in self.suggestion: self.draw_hex(q, r, outline="#BF616A", width=4)
        
        if self.hovered_hex and not self.game_over and not self.bot_playing:
            hq, hr = self.hovered_hex
            if self.hovered_hex in self.pieces or self.hovered_hex in self.current_turn_moves:
                self.draw_hex(hq, hr, fill="", outline="#FFFFFF", width=3) # Highlight border only for occupied
            else:
                self.draw_hex(hq, hr, fill="#4C566A", outline="#88C0D0", width=2) # Fill for empty spaces

        # Rounded exterior overlay for grid
        cr = 20
        self.canvas.create_polygon(cr, 0, cw - cr, 0, cw, 0, cw, cr, cw, ch - cr, cw, ch, cw - cr, ch, cr, ch, 0, ch, 0,
                                   ch - cr, 0, cr, 0, 0, fill="", outline="#A3BE8C", width=6, smooth=True)

        self.update_status()

    def draw_hex(self, q, r, fill="", outline="black", width=1, **kwargs):
        x = self.offset_x + self.hex_size * math.sqrt(3) * (q + r / 2.0)
        y = self.offset_y + self.hex_size * 3.0 / 2.0 * r
        pts = [val for i in range(6) for val in (x + self.hex_size * math.cos(math.radians(60 * i - 30)),
                                                 y + self.hex_size * math.sin(math.radians(60 * i - 30)))]
        self.canvas.create_polygon(pts, fill=fill, outline=outline, width=width, **kwargs)

    def on_click_start(self, event):
        self.pan_start = (event.x, event.y)
        self.is_panning = False

    def on_pan(self, event):
        if self.pan_start:
            dx, dy = event.x - self.pan_start[0], event.y - self.pan_start[1]
            if not self.is_panning and (abs(dx) > 5 or abs(dy) > 5):
                self.is_panning = True
            if self.is_panning:
                self.offset_x += dx
                self.offset_y += dy
                self.pan_start = (event.x, event.y)
                self.draw_board()

    def on_click_release(self, event):
        if not self.is_panning and self.pan_start:
            # It's a click, handle piece placement
            self.handle_click(event.x, event.y)

        self.pan_start = None
        self.is_panning = False
        self.on_mouse_move(event)  # Update hover immediately after click

    def on_mouse_move(self, event):
        if self.is_panning or self.bot_playing or self.game_over:
            if self.hovered_hex is not None:
                self.hovered_hex = None
                self.draw_board()
            return

        qr = self.pixel_to_hex(event.x, event.y)
        if qr != self.hovered_hex:
            self.hovered_hex = qr
            self.draw_board()

    def on_mouse_leave(self, event):
        if self.hovered_hex is not None:
            self.hovered_hex = None
            self.draw_board()

    def on_zoom(self, event):
        zoom_in = False
        if event.num == 4 or getattr(event, 'delta', 0) > 0:
            zoom_in = True
        elif event.num == 5 or getattr(event, 'delta', 0) < 0:
            zoom_in = False
        else:
            return

        scale = 1.1 if zoom_in else 0.9
        new_size = max(10, min(100, self.hex_size * scale))
        
        if new_size == self.hex_size: return
        
        ratio = new_size / self.hex_size
        self.hex_size = new_size
        self.offset_x = event.x - (event.x - self.offset_x) * ratio
        self.offset_y = event.y - (event.y - self.offset_y) * ratio
        
        self.draw_board()

    def pixel_to_hex(self, x, y):
        q = (math.sqrt(3) / 3.0 * (x - self.offset_x) - 1.0 / 3.0 * (y - self.offset_y)) / self.hex_size
        r = (2.0 / 3.0 * (y - self.offset_y)) / self.hex_size
        return self.axial_round(q, r)

    def axial_round(self, q, r):
        s = -q - r
        rq, rr, rs = round(q), round(r), round(s)
        q_diff, r_diff, s_diff = abs(rq - q), abs(rr - r), abs(rs - s)
        if q_diff > r_diff and q_diff > s_diff:
            rq = -rr - rs
        elif r_diff > s_diff:
            rr = -rq - rs
        return int(rq), int(rr)

    def handle_click(self, x, y):
        if self.bot_playing or self.game_over: return
        qr = self.pixel_to_hex(x, y)
        if qr in self.pieces: return
        if qr in self.current_turn_moves:
            self.current_turn_moves.remove(qr)
        else:
            self.current_turn_moves.append(qr)
            self.suggestion = None
            if len(self.current_turn_moves) == 2:
                self.commit_moves()
        self.draw_board()

    def commit_moves(self):
        c1, c2 = self.current_turn_moves
        self.write_engine(f"move {c1[0]} {c1[1]} {c2[0]} {c2[1]}")
        col_type = "Player 1 (White)" if self.player_turn == 0 else "Player 2 (Black)"
        self.pieces[c1] = self.colors[col_type]
        self.pieces[c2] = self.colors[col_type]
        self.history.append((col_type, [c1, c2], 0.0, None))
        self.redo_stack.clear()
        self.current_turn_moves = []
        self.player_turn = 1 - self.player_turn
        self.evaluation = 0.0
        self.suggestion = None
        self.check_for_win()

    def on_reset(self):
        self.reset_game_state()
        self.draw_board()

    def on_undo(self):
        if len(self.history) <= 1: return  # can't undo the initial white piece
        self.write_engine("undo")
        last_move = self.history.pop()
        self.redo_stack.append(last_move)
        for c in last_move[1]:
            del self.pieces[c]
        self.current_turn_moves = []
        self.player_turn = 1 - self.player_turn
        self.game_over = False
        self.evaluation = self.history[-1][2]
        self.suggestion = self.history[-1][3]
        self.draw_board()

    def on_redo(self):
        if not self.redo_stack or self.game_over: return
        item = self.redo_stack.pop()
        c1, c2 = item[1]
        self.write_engine(f"move {c1[0]} {c1[1]} {c2[0]} {c2[1]}")
        self.pieces[c1] = self.colors[item[0]]
        self.pieces[c2] = self.colors[item[0]]
        self.history.append(item)
        self.current_turn_moves = []
        self.player_turn = 1 - self.player_turn
        self.evaluation = item[2]
        self.suggestion = item[3]
        self.check_for_win()
        self.draw_board()

    def on_analyse(self):
        if self.game_over: return
        self.waiting_for_analysis = True
        self.write_engine("analyse")

    def on_bot_move(self):
        if self.game_over: return
        if self.suggestion:
            self.write_engine(
                f"move {self.suggestion[0][0]} {self.suggestion[0][1]} {self.suggestion[1][0]} {self.suggestion[1][1]}")
            self.apply_bot_move(self.suggestion)
        else:
            self.waiting_for_bot = True
            self.write_engine("moverequest")

    def apply_bot_move(self, res):
        c1, c2 = res
        col_type = "Player 1 (White)" if self.player_turn == 0 else "Player 2 (Black)"
        self.pieces[c1] = self.colors[col_type]
        self.pieces[c2] = self.colors[col_type]
        self.history.append((col_type, [c1, c2], 0.0, None))
        self.player_turn = 1 - self.player_turn
        self.suggestion = None
        self.evaluation = 0.0
        self.draw_board()
        if not self.check_for_win() and self.bot_playing:
            self.root.after(100, self.on_bot_move)

    def check_for_win(self):
        directions = [(1, 0), (0, 1), (1, -1)]
        for (q, r), color in self.pieces.items():
            for dq, dr in directions:
                count = 1
                cq, cr = q + dq, r + dr
                while self.pieces.get((cq, cr)) == color:
                    count += 1
                    cq += dq
                    cr += dr
                if count >= 6:
                    winner = next((p for p, c in self.colors.items() if c == color), "Player")
                    self.game_over = True
                    self.status_var.set(f"Game Over!\n{winner} Wins!")
                    if self.bot_playing: self.toggle_bot_vs_bot()
                    self.root.after(10, lambda: self.show_custom_popup("Game Over", f"{winner} has aligned 6 tiles!"))
                    return True
        return False

    def show_custom_popup(self, title, message):
        popup = tk.Toplevel(self.root)
        popup.title(title)
        popup.geometry("350x180")
        popup.configure(bg="#3B4252")
        popup.transient(self.root)
        popup.grab_set()

        self.root.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() // 2) - 175
        y = self.root.winfo_y() + (self.root.winfo_height() // 2) - 90
        popup.geometry(f"+{x}+{y}")

        tk.Label(popup, text=title, font=("Helvetica", 16, "bold"), bg="#3B4252", fg="#EBCB8B").pack(pady=(20, 10))
        tk.Label(popup, text=message, font=("Helvetica", 12), bg="#3B4252", fg="#ECEFF4", justify=tk.CENTER).pack(
            pady=(0, 20), padx=20)
        tk.Button(popup, text="OK", command=popup.destroy, bg="#4C566A", fg="#ECEFF4", font=("Helvetica", 12), bd=0,
                  activebackground="#434C5E", activeforeground="#FFFFFF", cursor="hand2", padx=30, pady=8).pack()

    def toggle_bot_vs_bot(self):
        self.bot_playing = not self.bot_playing
        if self.bot_playing:
            self.bot_vs_bot_btn.config(text="Bot vs Bot: ON", bg="#198754", activebackground="#20c997")
            self.on_bot_move()
        else:
            self.bot_vs_bot_btn.config(text="Bot vs Bot: OFF", bg="#dc3545", activebackground="#e4606d")


if __name__ == "__main__":
    root = tk.Tk()
    app = HexGameUI(root)
    root.mainloop()
