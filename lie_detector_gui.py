"""
lie_detector_gui.py — Interfaz principal del Detector de Mentiras Fisiológico
Physiological Lie Detector · Grupo 6K · CETYS Universidad 2026

Dependencias:
    pip install pyserial matplotlib numpy

Uso:
    python lie_detector_gui.py
"""

import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation
import numpy as np
import time
import collections
from uart_handler import UARTHandler

# ── Paleta de colores ──────────────────────────────────────────────────────────
BG        = "#0a0e1a"   # fondo principal
PANEL     = "#111827"   # paneles
ACCENT    = "#00f5d4"   # cian neón (principal)
ACCENT2   = "#f72585"   # magenta neón (alertas)
ACCENT3   = "#7209b7"   # morado (secundario)
TEXT      = "#e2e8f0"   # texto principal
MUTED     = "#64748b"   # texto secundario
SAFE      = "#06d6a0"   # verde (sin estrés)
WARN      = "#ffd166"   # amarillo (alerta)
DANGER    = "#ef233c"   # rojo (alto estrés)

GRAPH_BG  = "#0d1117"
GRID_CLR  = "#1e293b"

# ── Configuración de la gráfica ────────────────────────────────────────────────
WINDOW_SECONDS = 30       # segundos de historia visible
SAMPLE_RATE_HZ = 20       # Hz esperado del STM32
MAX_SAMPLES = WINDOW_SECONDS * SAMPLE_RATE_HZ

# ── Umbrales de estrés (ajustables) ───────────────────────────────────────────
BASELINE_SAMPLES = 100     # muestras para calcular baseline
STRESS_THRESHOLD_LOW  = 0.08   # +8%  sobre baseline → alerta
STRESS_THRESHOLD_HIGH = 0.20   # +20% sobre baseline → alto estrés


class LieDetectorApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Physiological Lie Detector · Grupo 6K")
        self.root.configure(bg=BG)
        self.root.minsize(1100, 680)

        # Estado de datos
        self.gsr_data   = collections.deque(maxlen=MAX_SAMPLES)
        self.time_data  = collections.deque(maxlen=MAX_SAMPLES)
        self.baseline   = None
        self.calibrating = False
        self.calib_samples: list[float] = []
        self.stress_level = "NORMAL"  # NORMAL | ALERTA | ALTO
        self.start_time = time.time()

        # UART
        self.uart = UARTHandler()

        # Construir UI
        self._build_ui()
        self._refresh_ports()

        # Animación matplotlib
        self.anim = FuncAnimation(
            self.fig, self._update_plot,
            interval=50,   # ms
            blit=False,
            cache_frame_data=False,
        )

        # Loop de polling de datos
        self._poll_data()

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ── Construcción de la UI ──────────────────────────────────────────────────
    def _build_ui(self):
        # ── Header ──
        header = tk.Frame(self.root, bg=BG, height=60)
        header.pack(fill="x", padx=0, pady=0)
        header.pack_propagate(False)

        tk.Label(
            header,
            text="⬡  PHYSIOLOGICAL LIE DETECTOR",
            bg=BG, fg=ACCENT,
            font=("Courier New", 16, "bold"),
        ).pack(side="left", padx=20, pady=10)

        tk.Label(
            header,
            text="Grupo 6K · Microcontroladores · CETYS 2026",
            bg=BG, fg=MUTED,
            font=("Courier New", 10),
        ).pack(side="left", padx=5, pady=10)

        # Indicador de estado (lado derecho del header)
        self.status_dot = tk.Label(header, text="●", bg=BG, fg=MUTED, font=("Courier New", 14))
        self.status_dot.pack(side="right", padx=5)
        self.status_label = tk.Label(header, text="DESCONECTADO", bg=BG, fg=MUTED, font=("Courier New", 10, "bold"))
        self.status_label.pack(side="right", padx=2)

        # Separador
        sep = tk.Frame(self.root, bg=ACCENT3, height=1)
        sep.pack(fill="x")

        # ── Cuerpo principal ──
        body = tk.Frame(self.root, bg=BG)
        body.pack(fill="both", expand=True, padx=15, pady=10)

        # Panel izquierdo (controles + métricas)
        left = tk.Frame(body, bg=PANEL, width=260, relief="flat")
        left.pack(side="left", fill="y", padx=(0, 12))
        left.pack_propagate(False)
        self._build_left_panel(left)

        # Panel derecho (gráfica)
        right = tk.Frame(body, bg=PANEL, relief="flat")
        right.pack(side="left", fill="both", expand=True)
        self._build_graph_panel(right)

    def _build_left_panel(self, parent):
        def section(text):
            tk.Label(parent, text=text, bg=PANEL, fg=ACCENT3,
                     font=("Courier New", 9, "bold")).pack(anchor="w", padx=15, pady=(14, 2))
            tk.Frame(parent, bg=ACCENT3, height=1).pack(fill="x", padx=15)

        # ── Conexión UART ──
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
        baud_combo = ttk.Combobox(parent, textvariable=self.baud_var, width=12,
                                   font=("Courier New", 9), state="readonly",
                                   values=["9600", "19200", "38400", "57600", "115200"])
        baud_combo.pack(anchor="w", padx=15)

        self.connect_btn = tk.Button(
            parent, text="CONECTAR", bg=ACCENT, fg=BG,
            font=("Courier New", 10, "bold"), relief="flat",
            command=self._toggle_connection, cursor="hand2",
            padx=10, pady=5,
        )
        self.connect_btn.pack(fill="x", padx=15, pady=8)

        self.demo_btn = tk.Button(
            parent, text="MODO DEMO  ▶", bg=PANEL, fg=ACCENT,
            font=("Courier New", 9), relief="flat",
            command=self._start_demo, cursor="hand2",
            padx=10, pady=4, bd=1,
        )
        self.demo_btn.pack(fill="x", padx=15)

        # ── Sensor GSR ──
        section("SENSOR GSR")

        metrics = [
            ("Raw ADC",    "gsr_val_lbl",  "---", TEXT),
            ("Filtrado",   "gsr_filt_lbl", "---", MUTED),
            ("R_piel",     "rskin_lbl",    "---", MUTED),
            ("Baseline",   "baseline_lbl", "---", MUTED),
            ("Δ relativo", "delta_lbl",    "---", MUTED),
            ("Muestras",   "samples_lbl",  "0",   MUTED),
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

        # ── Calibración ──
        section("CALIBRACIÓN")

        tk.Label(
            parent,
            text="Mantén 30 s en reposo\npara establecer baseline.",
            bg=PANEL, fg=MUTED,
            font=("Courier New", 8),
            justify="left",
        ).pack(anchor="w", padx=15, pady=(6, 4))

        self.calib_btn = tk.Button(
            parent, text="INICIAR CALIBRACIÓN", bg=ACCENT3, fg=TEXT,
            font=("Courier New", 9, "bold"), relief="flat",
            command=self._start_calibration, cursor="hand2",
            padx=10, pady=5,
        )
        self.calib_btn.pack(fill="x", padx=15)

        self.calib_bar_frame = tk.Frame(parent, bg=GRID_CLR, height=8)
        self.calib_bar_frame.pack(fill="x", padx=15, pady=4)
        self.calib_bar = tk.Frame(self.calib_bar_frame, bg=ACCENT3, height=8, width=0)
        self.calib_bar.place(x=0, y=0, height=8)

        # ── Nivel de estrés ──
        section("ANÁLISIS")

        self.stress_lbl = tk.Label(
            parent, text="NORMAL",
            bg=PANEL, fg=SAFE,
            font=("Courier New", 18, "bold"),
        )
        self.stress_lbl.pack(pady=10)

        self.stress_desc = tk.Label(
            parent,
            text="Sin perturbaciones\ndetectadas.",
            bg=PANEL, fg=MUTED,
            font=("Courier New", 8),
            justify="center",
        )
        self.stress_desc.pack()

        # ── Sensores futuros ──
        section("PRÓXIMOS SENSORES")
        for sensor, estado in [("HR (Pulso)", "Pendiente"), ("Temperatura", "Pendiente")]:
            row = tk.Frame(parent, bg=PANEL)
            row.pack(fill="x", padx=15, pady=1)
            tk.Label(row, text=sensor, bg=PANEL, fg=MUTED,
                     font=("Courier New", 8)).pack(side="left")
            tk.Label(row, text=estado, bg=PANEL, fg=ACCENT3,
                     font=("Courier New", 8)).pack(side="right")

    def _build_graph_panel(self, parent):
        # Título de la gráfica
        title_bar = tk.Frame(parent, bg=PANEL)
        title_bar.pack(fill="x", padx=15, pady=(10, 0))
        tk.Label(title_bar, text="SEÑAL GSR EN TIEMPO REAL",
                 bg=PANEL, fg=TEXT, font=("Courier New", 11, "bold")).pack(side="left")
        self.time_lbl = tk.Label(title_bar, text="T+0s", bg=PANEL, fg=MUTED,
                                  font=("Courier New", 9))
        self.time_lbl.pack(side="right")

        # Figura matplotlib
        self.fig, self.ax = plt.subplots(figsize=(8, 4.5), facecolor=GRAPH_BG)
        self.ax.set_facecolor(GRAPH_BG)
        self.ax.tick_params(colors=MUTED, labelsize=8)
        for spine in self.ax.spines.values():
            spine.set_edgecolor(GRID_CLR)
        self.ax.set_xlabel("Tiempo (s)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax.set_ylabel("ADC (0–4095)", color=MUTED, fontsize=9, fontfamily="monospace")
        self.ax.grid(True, color=GRID_CLR, linewidth=0.6, linestyle="--")
        self.ax.set_ylim(0, 4096)
        self.ax.set_xlim(-WINDOW_SECONDS, 0)

        # Líneas
        self.line_gsr,  = self.ax.plot([], [], color=ACCENT,  linewidth=1.4, label="GSR")
        self.line_base, = self.ax.plot([], [], color=WARN, linewidth=1.0,
                                        linestyle="--", label="Baseline", alpha=0.7)
        self.fill = None  # relleno bajo la curva

        # Leyenda
        self.ax.legend(loc="upper right", facecolor=PANEL, edgecolor=GRID_CLR,
                       labelcolor=TEXT, fontsize=8)

        # Canvas tkinter
        self.canvas = FigureCanvasTkAgg(self.fig, master=parent)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=10, pady=8)

        plt.tight_layout(pad=1.5)

    # ── Animación ─────────────────────────────────────────────────────────────
    def _update_plot(self, frame):
        if len(self.time_data) < 2:
            return

        t = np.array(self.time_data)
        g = np.array(self.gsr_data)
        t_rel = t - t[-1]  # relativo al último punto

        self.line_gsr.set_data(t_rel, g)

        # Línea de baseline
        if self.baseline is not None:
            self.line_base.set_data([t_rel[0], t_rel[-1]], [self.baseline, self.baseline])
            self.line_base.set_visible(True)

            # Relleno coloreado según nivel de estrés
            if self.fill:
                self.fill.remove()
            color_fill = {"NORMAL": SAFE, "ALERTA": WARN, "ALTO": DANGER}.get(self.stress_level, SAFE)
            self.fill = self.ax.fill_between(t_rel, g, self.baseline,
                                              alpha=0.08, color=color_fill)
        else:
            self.line_base.set_visible(False)

        # Ajustar eje X
        x_min = min(t_rel[0], -WINDOW_SECONDS)
        self.ax.set_xlim(x_min, 0)

        # Ajustar eje Y dinámico con margen
        if len(g) > 0:
            ymin = max(0, g.min() - 100)
            ymax = min(4096, g.max() + 100)
            self.ax.set_ylim(ymin, ymax)

        self.canvas.draw_idle()

    # ── Polling de datos ──────────────────────────────────────────────────────
    def _poll_data(self):
        processed = 0
        while not self.uart.data_queue.empty() and processed < 20:
            frame = self.uart.data_queue.get_nowait()
            processed += 1

            if frame.gsr is not None:
                now = time.time() - self.start_time
                self.gsr_data.append(frame.gsr)
                self.time_data.append(now)

                # Calibración en curso
                if self.calibrating:
                    self.calib_samples.append(frame.gsr)
                    pct = min(len(self.calib_samples) / BASELINE_SAMPLES, 1.0)
                    bar_w = int(pct * self.calib_bar_frame.winfo_width())
                    self.calib_bar.place(x=0, y=0, height=8, width=bar_w)
                    if len(self.calib_samples) >= BASELINE_SAMPLES:
                        self._finish_calibration()

                # Actualizar métricas pasando el frame completo
                self._update_metrics(frame)

        # Actualizar tiempo en header
        elapsed = int(time.time() - self.start_time)
        self.time_lbl.config(text=f"T+{elapsed}s")

        self.root.after(50, self._poll_data)  # 20 Hz

    def _update_metrics(self, frame):
        gsr = frame.gsr

        # ── Valores del firmware ──────────────────────────────────────────
        self.gsr_val_lbl.config(text=f"{gsr:.0f}")
        self.samples_lbl.config(text=str(len(self.gsr_data)))

        if frame.gsr_filt is not None:
            self.gsr_filt_lbl.config(text=f"{frame.gsr_filt:.0f}")

        if frame.r_skin is not None:
            r = frame.r_skin
            if r >= 1_000_000:
                self.rskin_lbl.config(text=f"{r/1_000_000:.2f} MΩ")
            elif r >= 1_000:
                self.rskin_lbl.config(text=f"{r/1_000:.1f} kΩ")
            else:
                self.rskin_lbl.config(text=f"{r:.0f} Ω")

        # ── Modo calibración: banner en lugar de estado ───────────────────
        if frame.cal_mode:
            self.stress_lbl.config(text="CALIBRANDO", fg=WARN)
            self.stress_desc.config(text="Gira el potenciómetro\nhasta minimizar Raw.")
            return

        # ── Estado directo del firmware (NORMAL / ACTIVO) ─────────────────
        fw_estado = frame.estado  # puede ser None si no vino del firmware
        if fw_estado is not None:
            if fw_estado == "ACTIVO":
                level, color, desc = "ACTIVO", DANGER, "¡Actividad simpática\ndetectada! (R_piel baja)"
            else:
                level, color, desc = "NORMAL", SAFE, "Sin perturbaciones\ndetectadas."

            if level != self.stress_level:
                self.stress_level = level
                self.stress_lbl.config(text=level, fg=color)
                self.stress_desc.config(text=desc)

        # ── Δ relativo respecto al baseline de Python (calibración GUI) ───
        if self.baseline is not None:
            delta = (gsr - self.baseline) / self.baseline if self.baseline else 0
            sign = "+" if delta >= 0 else ""
            self.delta_lbl.config(text=f"{sign}{delta*100:.1f}%")
            self.baseline_lbl.config(text=f"{self.baseline:.0f}")

            # Si el firmware no manda estado, calcular localmente
            if fw_estado is None:
                if abs(delta) >= STRESS_THRESHOLD_HIGH:
                    level, color, desc = "ALTO",   DANGER, "¡Perturbación\nalta detectada!"
                elif abs(delta) >= STRESS_THRESHOLD_LOW:
                    level, color, desc = "ALERTA",  WARN,   "Perturbación\nleve detectada."
                else:
                    level, color, desc = "NORMAL",  SAFE,   "Sin perturbaciones\ndetectadas."
                if level != self.stress_level:
                    self.stress_level = level
                    self.stress_lbl.config(text=level, fg=color)
                    self.stress_desc.config(text=desc)
            self.delta_lbl.config(fg={"NORMAL": SAFE, "ACTIVO": DANGER,
                                       "ALERTA": WARN, "ALTO": DANGER}.get(self.stress_level, MUTED))

    # ── Calibración ───────────────────────────────────────────────────────────
    def _start_calibration(self):
        if not self.uart.connected:
            messagebox.showwarning("Sin conexión", "Conecta el STM32 primero.")
            return
        self.calib_samples.clear()
        self.calibrating = True
        self.calib_btn.config(text="Calibrando…", state="disabled")
        self.uart.send_command("CALIBRATE")

    def _finish_calibration(self):
        self.calibrating = False
        self.baseline = float(np.mean(self.calib_samples))
        self.calib_btn.config(text="RE-CALIBRAR", state="normal")
        self.calib_bar.place(x=0, y=0, height=8,
                             width=self.calib_bar_frame.winfo_width())
        self.baseline_lbl.config(text=f"{self.baseline:.0f}")

    # ── Conexión ──────────────────────────────────────────────────────────────
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
                self.status_label.config(text=f"CONECTADO  {self.uart.port}", fg=SAFE)
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

    def _refresh_ports(self):
        ports = UARTHandler.list_ports()
        self.port_combo["values"] = ports
        if ports:
            # Intenta seleccionar automáticamente el STM32
            stm = UARTHandler.find_stm32_port()
            self.port_var.set(stm if stm else ports[0])

    def _on_close(self):
        self.uart.disconnect()
        plt.close("all")
        self.root.destroy()


# ── Entry point ───────────────────────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()

    # Estilo ttk (comboboxes oscuros)
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
