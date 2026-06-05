"""
lie_detector_gui.py — Interfaz visual del Detector de Mentiras Fisiológico
Physiological Lie Detector · Grupo 6K · CETYS Universidad 2026

ARQUITECTURA:
  STM32 hace: lectura ADC → filtro IIR → R_piel → calibración → LieP%
  Python hace: recibir frame → graficar R_piel → mostrar LieP% del MCU

  Gráfica superior: R_piel en Ω (resistencia real del sensor Grove GSR)
    - R alta  → persona relajada
    - R baja  → persona estresada / posible mentira
    - Línea verde  = Base Normal (calibrada)
    - Línea roja   = Base Mentira (calibrada)

Dependencias:
    pip install pyserial matplotlib numpy
"""

import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation
import numpy as np
import time
import collections
from uart_handler import UARTHandler

# ── Paleta de colores ──────────────────────────────────────────────────────────
BG        = "#0a0e1a"
PANEL     = "#111827"
ACCENT    = "#00f5d4"
ACCENT2   = "#f72585"
ACCENT3   = "#7209b7"
TEXT      = "#e2e8f0"
MUTED     = "#64748b"
SAFE      = "#06d6a0"
WARN      = "#ffd166"
DANGER    = "#ef233c"

GRAPH_BG  = "#0d1117"
GRID_CLR  = "#1e293b"

# ── Configuración de la gráfica ────────────────────────────────────────────────
WINDOW_SECONDS = 30
SAMPLE_RATE_HZ = 28          # ~28 Hz (35 ms por muestra en el MCU)
MAX_SAMPLES    = WINDOW_SECONDS * SAMPLE_RATE_HZ


class LieDetectorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Physiological Lie Detector · Grupo 6K")
        self.root.configure(bg=BG)
        self.root.minsize(1200, 800)

        # ── Buffers de señal ──────────────────────────────────────────────
        self.rskin_data    = collections.deque(maxlen=MAX_SAMPLES)  # R_piel (Ω)
        self.gsr_data      = collections.deque(maxlen=MAX_SAMPLES)  # ADC raw (interno)
        self.gsr_filt_data = collections.deque(maxlen=MAX_SAMPLES)  # ADC filt (interno)
        self.time_data     = collections.deque(maxlen=MAX_SAMPLES)
        self.hr_data       = collections.deque(maxlen=MAX_SAMPLES)
        self.hr_time_data  = collections.deque(maxlen=MAX_SAMPLES)
        self.lie_pct_data  = collections.deque(maxlen=MAX_SAMPLES)

        # ── Estado recibido del MCU ───────────────────────────────────────
        self.current_lie_pct = -1
        self.current_estado  = "SIN_CAL"
        self.base_normal     = None   # R_piel en reposo  (Ω) — del MCU
        self.base_lie        = None   # R_piel bajo estrés (Ω) — del MCU

        # ── Estado de calibración ─────────────────────────────────────────
        self.cal_normal_active = False
        self.cal_lie_active    = False
        self.cal_progress      = 0
        self.cal_total         = 150

        self.start_time = time.time()

        # UART
        self.uart = UARTHandler()

        # UI
        self._build_ui()
        self._refresh_ports()

        # Animación
        self.anim = FuncAnimation(
            self.fig, self._update_plot,
            interval=50,
            blit=False,
            cache_frame_data=False,
        )

        self._poll_data()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ── UI ─────────────────────────────────────────────────────────────────────
    def _build_ui(self):
        header = tk.Frame(self.root, bg=BG, height=60)
        header.pack(fill="x")
        header.pack_propagate(False)

        tk.Label(header, text="⬡  PHYSIOLOGICAL LIE DETECTOR",
                 bg=BG, fg=ACCENT, font=("Courier New", 16, "bold")).pack(side="left", padx=20, pady=10)
        tk.Label(header, text="Grupo 6K · Microcontroladores · CETYS 2026",
                 bg=BG, fg=MUTED, font=("Courier New", 10)).pack(side="left", padx=5)

        self.status_dot   = tk.Label(header, text="●", bg=BG, fg=MUTED, font=("Courier New", 14))
        self.status_dot.pack(side="right", padx=5)
        self.status_label = tk.Label(header, text="DESCONECTADO", bg=BG, fg=MUTED,
                                     font=("Courier New", 10, "bold"))
        self.status_label.pack(side="right", padx=2)

        tk.Frame(self.root, bg=ACCENT3, height=1).pack(fill="x")

        body = tk.Frame(self.root, bg=BG)
        body.pack(fill="both", expand=True, padx=15, pady=10)

        left = tk.Frame(body, bg=PANEL, width=280, relief="flat")
        left.pack(side="left", fill="y", padx=(0, 12))
        left.pack_propagate(False)
        self._build_left_panel(left)

        right = tk.Frame(body, bg=PANEL, relief="flat")
        right.pack(side="left", fill="both", expand=True)
        self._build_graph_panel(right)

    def _build_left_panel(self, parent):
        def section(text):
            tk.Label(parent, text=text, bg=PANEL, fg=ACCENT3,
                     font=("Courier New", 9, "bold")).pack(anchor="w", padx=15, pady=(14, 2))
            tk.Frame(parent, bg=ACCENT3, height=1).pack(fill="x", padx=15)

        # ── Conexión ──────────────────────────────────────────────────────
        section("CONEXIÓN UART")

        tk.Label(parent, text="Puerto:", bg=PANEL, fg=TEXT,
                 font=("Courier New", 9)).pack(anchor="w", padx=15, pady=(8, 0))

        port_row = tk.Frame(parent, bg=PANEL)
        port_row.pack(fill="x", padx=15)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_row, textvariable=self.port_var, width=14,
                                       font=("Courier New", 9), state="readonly")
        self.port_combo.pack(side="left")
        tk.Button(port_row, text="↺", bg=ACCENT3, fg=TEXT, relief="flat",
                  font=("Courier New", 10), command=self._refresh_ports,
                  cursor="hand2", padx=5).pack(side="left", padx=4)

        tk.Label(parent, text="Baud rate:", bg=PANEL, fg=TEXT,
                 font=("Courier New", 9)).pack(anchor="w", padx=15, pady=(6, 0))
        self.baud_var = tk.StringVar(value="115200")
        ttk.Combobox(parent, textvariable=self.baud_var, width=12,
                     font=("Courier New", 9), state="readonly",
                     values=["9600", "19200", "38400", "57600", "115200"]).pack(anchor="w", padx=15)

        self.connect_btn = tk.Button(parent, text="CONECTAR", bg=ACCENT, fg=BG,
                                     font=("Courier New", 10, "bold"), relief="flat",
                                     command=self._toggle_connection, cursor="hand2",
                                     padx=10, pady=5)
        self.connect_btn.pack(fill="x", padx=15, pady=8)

        self.demo_btn = tk.Button(parent, text="MODO DEMO  ▶", bg=PANEL, fg=ACCENT,
                                  font=("Courier New", 9), relief="flat",
                                  command=self._start_demo, cursor="hand2",
                                  padx=10, pady=4, bd=1)
        self.demo_btn.pack(fill="x", padx=15)

        # ── Métricas GSR ──────────────────────────────────────────────────
        section("SENSOR GSR  (procesado en STM32)")
        metrics = [
            ("Raw ADC",   "gsr_val_lbl",  "---", TEXT),
            ("Filtrado",  "gsr_filt_lbl", "---", MUTED),
            ("R_piel",    "rskin_lbl",    "---", ACCENT),
            ("Muestras",  "samples_lbl",  "0",   MUTED),
        ]
        for label, attr, default, color in metrics:
            row = tk.Frame(parent, bg=PANEL)
            row.pack(fill="x", padx=15, pady=2)
            tk.Label(row, text=label + ":", bg=PANEL, fg=MUTED,
                     font=("Courier New", 8), width=11, anchor="w").pack(side="left")
            lbl = tk.Label(row, text=default, bg=PANEL, fg=color,
                           font=("Courier New", 10, "bold"), anchor="e")
            lbl.pack(side="right")
            setattr(self, attr, lbl)

        # ── Métricas HR ───────────────────────────────────────────────────
        section("SENSOR PULSO  (procesado en STM32)")
        hr_metrics = [
            ("HR Raw",  "hr_val_lbl", "---", ACCENT2),
            ("BPM",     "bpm_lbl",    "---", ACCENT2),
        ]
        for label, attr, default, color in hr_metrics:
            row = tk.Frame(parent, bg=PANEL)
            row.pack(fill="x", padx=15, pady=2)
            tk.Label(row, text=label + ":", bg=PANEL, fg=MUTED,
                     font=("Courier New", 8), width=11, anchor="w").pack(side="left")
            lbl = tk.Label(row, text=default, bg=PANEL, fg=color,
                           font=("Courier New", 10, "bold"), anchor="e")
            lbl.pack(side="right")
            setattr(self, attr, lbl)

        # ── Calibración en el MCU ─────────────────────────────────────────
        section("CALIBRACIÓN  (ejecutada en STM32)")

        tk.Label(parent,
                 text="Calibra en reposo → NORMAL\nCalibra bajo estrés → MENTIRA",
                 bg=PANEL, fg=MUTED, font=("Courier New", 8), justify="left"
                 ).pack(anchor="w", padx=15, pady=(4, 6))

        self.calib_normal_btn = tk.Button(
            parent, text="CALIBRAR → NORMAL", bg=SAFE, fg=BG,
            font=("Courier New", 9, "bold"), relief="flat",
            command=self._start_cal_normal, cursor="hand2", padx=8, pady=4)
        self.calib_normal_btn.pack(fill="x", padx=15, pady=(2, 1))

        self.calib_lie_btn = tk.Button(
            parent, text="CALIBRAR → MENTIRA", bg=DANGER, fg=TEXT,
            font=("Courier New", 9, "bold"), relief="flat",
            command=self._start_cal_lie, cursor="hand2", padx=8, pady=4)
        self.calib_lie_btn.pack(fill="x", padx=15, pady=(1, 4))

        self.calib_bar_frame = tk.Frame(parent, bg=GRID_CLR, height=8)
        self.calib_bar_frame.pack(fill="x", padx=15, pady=2)
        self.calib_bar = tk.Frame(self.calib_bar_frame, bg=ACCENT3, height=8, width=0)
        self.calib_bar.place(x=0, y=0, height=8)

        # Bases calibradas (en Ω — mismas unidades que la gráfica)
        row = tk.Frame(parent, bg=PANEL)
        row.pack(fill="x", padx=15, pady=1)
        tk.Label(row, text="Base Normal:", bg=PANEL, fg=MUTED,
                 font=("Courier New", 8), width=11, anchor="w").pack(side="left")
        self.base_normal_lbl = tk.Label(row, text="---", bg=PANEL, fg=SAFE,
                                        font=("Courier New", 9, "bold"), anchor="e")
        self.base_normal_lbl.pack(side="right")

        row2 = tk.Frame(parent, bg=PANEL)
        row2.pack(fill="x", padx=15, pady=1)
        tk.Label(row2, text="Base Mentira:", bg=PANEL, fg=MUTED,
                 font=("Courier New", 8), width=11, anchor="w").pack(side="left")
        self.base_lie_lbl = tk.Label(row2, text="---", bg=PANEL, fg=DANGER,
                                     font=("Courier New", 9, "bold"), anchor="e")
        self.base_lie_lbl.pack(side="right")

        # ── Probabilidad de mentira (del MCU) ────────────────────────────
        section("PROBABILIDAD DE MENTIRA  (del STM32)")

        self.lie_pct_lbl = tk.Label(parent, text="---", bg=PANEL, fg=SAFE,
                                    font=("Courier New", 28, "bold"))
        self.lie_pct_lbl.pack(pady=(6, 0))

        self.estado_lbl = tk.Label(parent, text="SIN CALIBRAR", bg=PANEL, fg=MUTED,
                                   font=("Courier New", 13, "bold"))
        self.estado_lbl.pack()

        self.estado_desc = tk.Label(parent,
                                    text="Calibra primero en el MCU\npresionando los botones.",
                                    bg=PANEL, fg=MUTED, font=("Courier New", 8), justify="center")
        self.estado_desc.pack(pady=(2, 8))

    def _build_graph_panel(self, parent):
        title_bar = tk.Frame(parent, bg=PANEL)
        title_bar.pack(fill="x", padx=15, pady=(10, 0))
        tk.Label(title_bar, text="SEÑALES EN TIEMPO REAL  (filtradas por STM32)",
                 bg=PANEL, fg=TEXT, font=("Courier New", 11, "bold")).pack(side="left")
        self.time_lbl = tk.Label(title_bar, text="T+0s", bg=PANEL, fg=MUTED,
                                  font=("Courier New", 9))
        self.time_lbl.pack(side="right")

        self.fig, (self.ax, self.ax_hr, self.ax_lie) = plt.subplots(
            3, 1, figsize=(8, 7), facecolor=GRAPH_BG
        )

        for ax in (self.ax, self.ax_hr, self.ax_lie):
            ax.set_facecolor(GRAPH_BG)
            ax.tick_params(colors=MUTED, labelsize=8)
            for spine in ax.spines.values():
                spine.set_edgecolor(GRID_CLR)
            ax.grid(True, color=GRID_CLR, linewidth=0.6, linestyle="--")

        # ── R_piel ────────────────────────────────────────────────────────
        # Eje Y en kΩ para mejor legibilidad (se convierte en _update_plot)
        self.ax.set_ylabel("R_piel (kΩ)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax.set_ylim(0, 2000)          # 0–2000 kΩ = 0–2 MΩ inicial
        self.ax.set_xlim(-WINDOW_SECONDS, 0)

        # ── HR ────────────────────────────────────────────────────────────
        self.ax_hr.set_ylabel("HR ADC (0–4095)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax_hr.set_ylim(0, 4096)
        self.ax_hr.set_xlim(-WINDOW_SECONDS, 0)

        # ── LieP% ─────────────────────────────────────────────────────────
        self.ax_lie.set_ylabel("Prob. Mentira (%)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax_lie.set_xlabel("Tiempo (s)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax_lie.set_ylim(-5, 105)
        self.ax_lie.set_xlim(-WINDOW_SECONDS, 0)

        # ── Líneas ────────────────────────────────────────────────────────
        # R_piel principal (en kΩ)
        self.line_rskin,  = self.ax.plot([], [], color=ACCENT,  linewidth=1.8,
                                          label="R_piel (kΩ)")
        # Líneas horizontales de bases calibradas (en kΩ)
        self.line_base_n, = self.ax.plot([], [], color=SAFE,   linewidth=1.2,
                                          linestyle="--", label="Base Normal", alpha=0.9)
        self.line_base_l, = self.ax.plot([], [], color=DANGER, linewidth=1.2,
                                          linestyle="--", label="Base Mentira", alpha=0.9)
        # HR
        self.line_hr,     = self.ax_hr.plot([], [], color=ACCENT2, linewidth=1.4, label="HR")
        # LieP%
        self.line_lie,    = self.ax_lie.plot([], [], color=WARN, linewidth=1.8, label="LieP%")

        # Zonas de color en LieP
        self.ax_lie.axhspan(70, 105, alpha=0.07, color=DANGER)
        self.ax_lie.axhspan(40, 70,  alpha=0.07, color=WARN)
        self.ax_lie.axhspan(-5, 40,  alpha=0.07, color=SAFE)
        self.ax_lie.axhline(70, color=DANGER, linewidth=0.6, linestyle=":")
        self.ax_lie.axhline(40, color=WARN,   linewidth=0.6, linestyle=":")

        for ax, title in [(self.ax, "R_piel"), (self.ax_hr, "HR"), (self.ax_lie, "LieP%")]:
            ax.legend(loc="upper right", facecolor=PANEL, edgecolor=GRID_CLR,
                      labelcolor=TEXT, fontsize=7)

        self.canvas = FigureCanvasTkAgg(self.fig, master=parent)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=10, pady=8)

        plt.tight_layout(pad=1.2)

    # ── Animación ──────────────────────────────────────────────────────────────
    def _update_plot(self, frame):

        # ── R_piel ────────────────────────────────────────────────────────
        if len(self.time_data) >= 2 and len(self.rskin_data) >= 2:
            t      = np.array(self.time_data)
            rs_ohm = np.array(self.rskin_data, dtype=float)
            t_rel  = t - t[-1]

            # Convertir Ω → kΩ para la gráfica
            rs_kohm = rs_ohm / 1000.0

            self.line_rskin.set_data(t_rel, rs_kohm)

            # Líneas horizontales de bases (también en kΩ)
            if self.base_normal is not None:
                bn_k = self.base_normal / 1000.0
                self.line_base_n.set_data([t_rel[0], t_rel[-1]], [bn_k, bn_k])
                self.line_base_n.set_visible(True)
            else:
                self.line_base_n.set_visible(False)

            if self.base_lie is not None:
                bl_k = self.base_lie / 1000.0
                self.line_base_l.set_data([t_rel[0], t_rel[-1]], [bl_k, bl_k])
                self.line_base_l.set_visible(True)
            else:
                self.line_base_l.set_visible(False)

            self.ax.set_xlim(min(t_rel[0], -WINDOW_SECONDS), 0)

            # Autoescala: ignorar valores >= 1000 kΩ (sin dedo / saturado)
            rs_valid = rs_kohm[rs_kohm < 1000.0]
            if len(rs_valid) > 0:
                y_min = max(0.0,    rs_valid.min() - rs_valid.min() * 0.1)
                y_max = min(2000.0, rs_valid.max() + rs_valid.max() * 0.1)
                if y_max > y_min:
                    self.ax.set_ylim(y_min, y_max)

        # ── HR ───────────────────────────────────────────────────────────
        if len(self.hr_time_data) >= 2:
            t_hr  = np.array(self.hr_time_data)
            hr    = np.array(self.hr_data)
            t_rel = t_hr - t_hr[-1]
            self.line_hr.set_data(t_rel, hr)
            self.ax_hr.set_xlim(min(t_rel[0], -WINDOW_SECONDS), 0)
            if len(hr):
                self.ax_hr.set_ylim(max(0, hr.min() - 80), min(4096, hr.max() + 80))

        # ── LieP% ────────────────────────────────────────────────────────
        if len(self.time_data) >= 2 and len(self.lie_pct_data) >= 2:
            t     = np.array(self.time_data)
            lp    = np.array(self.lie_pct_data, dtype=float)
            t_rel = t - t[-1]
            lp[lp < 0] = np.nan   # ocultar puntos sin calibración
            self.line_lie.set_data(t_rel, lp)
            self.ax_lie.set_xlim(min(t_rel[0], -WINDOW_SECONDS), 0)

        self.canvas.draw_idle()

    # ── Polling ────────────────────────────────────────────────────────────────
    def _poll_data(self):
        processed = 0
        while not self.uart.data_queue.empty() and processed < 20:
            frame = self.uart.data_queue.get_nowait()
            processed += 1
            now = time.time() - self.start_time

            # ── Fin de calibración (notificación del MCU) ─────────────
            if frame.cal_done:
                if frame.cal_done == "NORMAL" and frame.cal_base is not None:
                    self.base_normal = frame.cal_base   # en Ω
                    self.base_normal_lbl.config(
                        text=self._fmt_ohm(frame.cal_base))
                    self.cal_normal_active = False
                    self.calib_normal_btn.config(state="normal",
                                                  text="CALIBRAR → NORMAL")
                    self.calib_bar.place(x=0, y=0, height=8, width=0)
                elif frame.cal_done == "LIE" and frame.cal_base is not None:
                    self.base_lie = frame.cal_base      # en Ω
                    self.base_lie_lbl.config(
                        text=self._fmt_ohm(frame.cal_base))
                    self.cal_lie_active = False
                    self.calib_lie_btn.config(state="normal",
                                               text="CALIBRAR → MENTIRA")
                    self.calib_bar.place(x=0, y=0, height=8, width=0)
                continue

            # ── Progreso de calibración ───────────────────────────────
            if (frame.cal_normal or frame.cal_lie) and frame.cal_progress is not None:
                total = frame.cal_total or self.cal_total
                pct   = min(frame.cal_progress / total, 1.0)
                bar_w = int(pct * self.calib_bar_frame.winfo_width())
                self.calib_bar.place(x=0, y=0, height=8, width=bar_w)
                if frame.gsr is not None:
                    self.gsr_data.append(frame.gsr)
                    self.gsr_filt_data.append(
                        frame.gsr if frame.gsr_filt is None else frame.gsr_filt)
                    self.rskin_data.append(
                        frame.r_skin if frame.r_skin is not None else 0.0)
                    self.time_data.append(now)
                    self.lie_pct_data.append(-1)
                if frame.hr is not None:
                    self.hr_data.append(frame.hr)
                    self.hr_time_data.append(now)
                continue

            # ── Calibración potenciómetro [CAL] ──────────────────────
            if frame.cal_mode:
                if frame.gsr is not None:
                    self.gsr_data.append(frame.gsr)
                    self.gsr_filt_data.append(frame.gsr)
                    self.rskin_data.append(0.0)
                    self.time_data.append(now)
                    self.lie_pct_data.append(-1)
                if frame.hr is not None:
                    self.hr_data.append(frame.hr)
                    self.hr_time_data.append(now)
                self._update_labels_cal(frame)
                continue

            # ── Frame normal ──────────────────────────────────────────
            if frame.gsr is not None:
                self.gsr_data.append(frame.gsr)
                self.gsr_filt_data.append(
                    frame.gsr_filt if frame.gsr_filt is not None else frame.gsr)
                self.rskin_data.append(
                    frame.r_skin if frame.r_skin is not None else 0.0)
                self.time_data.append(now)
                lp = frame.lie_pct if frame.lie_pct is not None else -1
                self.lie_pct_data.append(lp)

            if frame.hr is not None:
                self.hr_data.append(frame.hr)
                self.hr_time_data.append(now)

            self._update_labels(frame)

        elapsed = int(time.time() - self.start_time)
        self.time_lbl.config(text=f"T+{elapsed}s")
        self.root.after(50, self._poll_data)

    # ── Helpers ────────────────────────────────────────────────────────────────
    @staticmethod
    def _fmt_ohm(r: float) -> str:
        """Formatea una resistencia en Ω a string legible."""
        if r >= 1_000_000:
            return f"{r/1_000_000:.2f} MΩ"
        elif r >= 1_000:
            return f"{r/1_000:.1f} kΩ"
        else:
            return f"{r:.0f} Ω"

    def _update_labels_cal(self, frame):
        """Actualiza etiquetas en modo calibración potenciómetro."""
        if frame.gsr is not None:
            self.gsr_val_lbl.config(text=f"{frame.gsr:.0f}")
            self.samples_lbl.config(text=str(len(self.gsr_data)))
        if frame.hr is not None:
            self.hr_val_lbl.config(text=f"{frame.hr:.0f}")
        self.estado_lbl.config(text="POT CAL", fg=WARN)
        self.estado_desc.config(text="Gira el potenciómetro\nhasta minimizar Raw.")

    def _update_labels(self, frame):
        """Actualiza etiquetas con datos del frame normal."""
        if frame.gsr is not None:
            self.gsr_val_lbl.config(text=f"{frame.gsr:.0f}")
            self.samples_lbl.config(text=str(len(self.gsr_data)))
        if frame.gsr_filt is not None:
            self.gsr_filt_lbl.config(text=f"{frame.gsr_filt:.0f}")
        if frame.r_skin is not None:
            self.rskin_lbl.config(text=self._fmt_ohm(frame.r_skin))
        if frame.hr is not None:
            self.hr_val_lbl.config(text=f"{frame.hr:.0f}")
        if frame.bpm is not None and frame.bpm > 0:
            self.bpm_lbl.config(text=f"{frame.bpm:.0f} BPM")

        lie_pct = frame.lie_pct
        estado  = frame.estado or "SIN_CAL"

        if lie_pct is None or lie_pct < 0:
            self.lie_pct_lbl.config(text="---", fg=MUTED)
            self.estado_lbl.config(text="SIN CALIBRAR", fg=MUTED)
            self.estado_desc.config(text="Calibra primero con\nlos botones superiores.")
        else:
            self.current_lie_pct = lie_pct
            if lie_pct >= 70:
                color = DANGER
            elif lie_pct >= 40:
                color = WARN
            else:
                color = SAFE

            self.lie_pct_lbl.config(text=f"{lie_pct}%", fg=color)

            estado_map = {
                "MENTIRA":  (DANGER, "¡Alta probabilidad de\nrespuesta no verdadera!"),
                "SOSPECHA": (WARN,   "Respuesta sospechosa.\nActividad simpática elevada."),
                "NORMAL":   (SAFE,   "Sin indicadores de\ndeception detectados."),
                "SIN_CAL":  (MUTED,  "Sin datos de calibración."),
            }
            ec, ed = estado_map.get(estado, (MUTED, estado))
            self.estado_lbl.config(text=estado, fg=ec)
            self.estado_desc.config(text=ed)

    # ── Comandos de calibración al MCU ─────────────────────────────────────────
    def _start_cal_normal(self):
        if not self.uart.connected:
            messagebox.showwarning("Sin conexión", "Conecta el STM32 primero.")
            return
        self.cal_normal_active = True
        self.calib_normal_btn.config(text="Calibrando NORMAL…", state="disabled")
        self.uart.send_command("CAL_NORMAL")

    def _start_cal_lie(self):
        if not self.uart.connected:
            messagebox.showwarning("Sin conexión", "Conecta el STM32 primero.")
            return
        self.cal_lie_active = True
        self.calib_lie_btn.config(text="Calibrando MENTIRA…", state="disabled")
        self.uart.send_command("CAL_LIE")

    # ── Conexión ───────────────────────────────────────────────────────────────
    def _toggle_connection(self):
        if self.uart.connected:
            self.uart.disconnect()
            self.connect_btn.config(text="CONECTAR", bg=ACCENT)
            self.status_dot.config(fg=MUTED)
            self.status_label.config(text="DESCONECTADO", fg=MUTED)
        else:
            port = self.port_var.get()
            baud = int(self.baud_var.get())
            ok = self.uart.connect(port=port or None, baud=baud)
            if ok:
                self.connect_btn.config(text="DESCONECTAR", bg=ACCENT2)
                self.status_dot.config(fg=SAFE)
                self.status_label.config(
                    text=f"CONECTADO  {self.uart.port}", fg=SAFE)
            else:
                messagebox.showerror("Error de conexión", self.uart.error_message)

    def _start_demo(self):
        if self.uart.connected:
            self.uart.disconnect()
        self.uart = UARTHandler()
        self.uart.start_demo_mode()
        self.connect_btn.config(text="DESCONECTAR", bg=ACCENT2)
        self.status_dot.config(fg=WARN)
        self.status_label.config(text="MODO DEMO", fg=WARN)
        self.demo_btn.config(state="disabled")
        # En demo, simular calibración ya hecha (en Ω)
        self.base_normal = 500_000.0
        self.base_lie    = 100_000.0
        self.base_normal_lbl.config(text="500.0 kΩ (demo)")
        self.base_lie_lbl.config(text="100.0 kΩ (demo)")

    def _refresh_ports(self):
        ports = UARTHandler.list_ports()
        self.port_combo["values"] = ports
        if ports:
            stm = UARTHandler.find_stm32_port()
            self.port_var.set(stm if stm else ports[0])

    def _on_close(self):
        self.uart.disconnect()
        plt.close("all")
        self.root.destroy()


# ── Entry point ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()

    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TCombobox",
                    fieldbackground=GRAPH_BG,
                    background=GRAPH_BG,
                    foreground=TEXT,
                    selectbackground=ACCENT3,
                    selectforeground=TEXT,
                    bordercolor=ACCENT3,
                    arrowcolor=ACCENT)

    app = LieDetectorApp(root)
    root.mainloop()