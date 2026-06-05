"""
uart_handler.py — Módulo de comunicación UART con STM32
Physiological Lie Detector · Grupo 6K · CETYS Universidad 2026

RESPONSABILIDAD: Solo recibir y parsear el frame que ya procesó el MCU.
El filtrado, calibración fisiológica y cálculo de LieP se hacen en el STM32.
"""

import serial
import serial.tools.list_ports
import threading
import queue
import time
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class SensorFrame:
    """Frame de datos ya procesados por el STM32."""
    timestamp:  float          = field(default_factory=time.time)
    gsr:        Optional[float] = None   # Raw ADC GSR
    gsr_filt:   Optional[float] = None   # Filtrado IIR (hecho en MCU)
    r_skin:     Optional[float] = None   # Resistencia piel (Ω)
    hr:         Optional[float] = None   # HR Raw / filtrado
    bpm:        Optional[float] = None   # BPM calculado en MCU
    lie_pct:    Optional[int]   = None   # Probabilidad de mentira 0–100 (-1=sin cal)
    estado:     Optional[str]   = None   # "NORMAL"|"SOSPECHA"|"MENTIRA"|"SIN_CAL"
    cal_mode:   bool            = False  # [CAL] calibración potenciómetro
    cal_normal: bool            = False  # [CALN] calibración preguntas normales
    cal_lie:    bool            = False  # [CALL] calibración preguntas mentira
    cal_done:   Optional[str]   = None   # "NORMAL"|"LIE" cuando termina cal
    cal_base:   Optional[float] = None   # Valor base calculado al finalizar
    cal_progress: Optional[int] = None  # Progreso calibración (0–150)
    cal_total:  Optional[int]   = None  # Total muestras calibración
    raw:        str             = ""


class UARTHandler:
    DEFAULT_BAUD = 115200
    TIMEOUT = 1.0

    def __init__(self, port: str = None, baud: int = DEFAULT_BAUD):
        self.port = port
        self.baud = baud
        self.serial: Optional[serial.Serial] = None
        self.data_queue: queue.Queue[SensorFrame] = queue.Queue(maxsize=500)
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self.connected = False
        self.error_message = ""

    @staticmethod
    def list_ports() -> list[str]:
        ports = serial.tools.list_ports.comports()
        return [p.device for p in sorted(ports)]

    @staticmethod
    def find_stm32_port() -> Optional[str]:
        STM32_VIDS = {0x0483}
        for p in serial.tools.list_ports.comports():
            if p.vid in STM32_VIDS:
                return p.device
        return None

    def connect(self, port: str = None, baud: int = None) -> bool:
        if port:
            self.port = port
        if baud:
            self.baud = baud
        if not self.port:
            self.port = self.find_stm32_port()
        if not self.port:
            self.error_message = "No se encontró ningún puerto. Selecciona uno manualmente."
            return False
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                timeout=self.TIMEOUT,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
            )
            self.serial.reset_input_buffer()
            self.connected = True
            self.error_message = ""
            self._start_reader()
            return True
        except serial.SerialException as e:
            self.error_message = f"Error al abrir {self.port}: {e}"
            self.connected = False
            return False

    def disconnect(self):
        self._running = False
        if self._thread:
            self._thread.join(timeout=2)
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.connected = False

    def _start_reader(self):
        self._running = True
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def _read_loop(self):
        while self._running:
            try:
                if self.serial and self.serial.is_open:
                    raw = self.serial.readline()
                    if raw:
                        line = raw.decode("utf-8", errors="ignore").strip()
                        frame = self._parse_line(line)
                        if frame:
                            try:
                                self.data_queue.put_nowait(frame)
                            except queue.Full:
                                self.data_queue.get_nowait()
                                self.data_queue.put_nowait(frame)
            except serial.SerialException:
                self.connected = False
                self._running = False
                self.error_message = "Conexión serial perdida."
                break
            except Exception:
                pass

    @staticmethod
    def _parse_line(line: str) -> Optional["SensorFrame"]:
        """
        Parsea los formatos emitidos por el STM32:

        1. [CAL] GSR_Raw: 312 | HR_Raw: 2048         ← calibración potenciómetro
        2. [CALN] GSR_Raw: %d | HR_Raw: %d | Progress: %d/%d  ← cal normal
        3. [CALL] GSR_Raw: %d | HR_Raw: %d | Progress: %d/%d  ← cal mentira
        4. [CALN_DONE] BaseNorm:%d
        5. [CALL_DONE] BaseLie:%d
        6. Raw:%d|Filt:%d|Rpiel:%d|HR:%d|BPM:%d|LieP:%d|Estado:%s  ← normal
        """
        if not line:
            return None

        frame = SensorFrame(raw=line)

        # ── Formato 1: [CAL] calibración potenciómetro ────────────────
        if line.startswith("[CAL]"):
            frame.cal_mode = True
            for part in [p.strip() for p in line.split("|")]:
                if "GSR_Raw:" in part:
                    try:
                        frame.gsr = float(part.split("GSR_Raw:")[-1].strip().split()[0])
                    except (IndexError, ValueError):
                        pass
                elif "HR_Raw:" in part:
                    try:
                        frame.hr = float(part.split("HR_Raw:")[-1].strip().split()[0])
                    except (IndexError, ValueError):
                        pass
            return frame if frame.gsr is not None else None

        # ── Formato 2/3: [CALN]/[CALL] calibración fisiológica ────────
        if line.startswith("[CALN]") or line.startswith("[CALL]"):
            frame.cal_normal = line.startswith("[CALN]")
            frame.cal_lie    = line.startswith("[CALL]")
            for part in [p.strip() for p in line.split("|")]:
                if "GSR_Raw:" in part:
                    try:
                        frame.gsr = float(part.split("GSR_Raw:")[-1].strip().split()[0])
                    except (IndexError, ValueError):
                        pass
                elif "HR_Raw:" in part:
                    try:
                        frame.hr = float(part.split("HR_Raw:")[-1].strip().split()[0])
                    except (IndexError, ValueError):
                        pass
                elif "Rpiel:" in part:
                    try:
                        frame.r_skin = float(part.split("Rpiel:")[-1].strip().split()[0])
                    except (IndexError, ValueError):
                        pass
                elif "Progress:" in part:
                    try:
                        prog_str = part.split("Progress:")[-1].strip()
                        parts2   = prog_str.split("/")
                        frame.cal_progress = int(parts2[0])
                        frame.cal_total    = int(parts2[1])
                    except (IndexError, ValueError):
                        pass
            return frame if frame.gsr is not None else None

        # ── Formato 4/5: [CALN_DONE] / [CALL_DONE] ────────────────────
        if line.startswith("[CALN_DONE]"):
            frame.cal_done = "NORMAL"
            try:
                frame.cal_base = float(line.split("BaseNorm:")[-1].strip())
            except (IndexError, ValueError):
                pass
            return frame

        if line.startswith("[CALL_DONE]"):
            frame.cal_done = "LIE"
            try:
                frame.cal_base = float(line.split("BaseLie:")[-1].strip())
            except (IndexError, ValueError):
                pass
            return frame

        # ── Formato 6: Frame normal con LieP (emitido por MCU) ────────
        # Raw:%d|Filt:%d|Rpiel:%d|HR:%d|BPM:%d|LieP:%d|Estado:%s
        if line.startswith("Raw:"):
            parts = [p.strip() for p in line.split("|")]
            for part in parts:
                k, _, v = part.partition(":")
                k = k.strip()
                v = v.strip()
                try:
                    if k == "Raw":
                        frame.gsr = float(v)
                    elif k == "Filt":
                        frame.gsr_filt = float(v)
                    elif k == "Rpiel":
                        frame.r_skin = float(v)
                    elif k == "HR":
                        frame.hr = float(v)
                    elif k == "BPM":
                        frame.bpm = float(v)
                    elif k == "LieP":
                        val = int(float(v))
                        frame.lie_pct = val  # -1 = sin calibrar
                    elif k == "Estado":
                        frame.estado = v     # ya calculado en MCU
                except ValueError:
                    pass
            return frame if frame.gsr is not None else None

        # ── Fallback: valor crudo numérico ─────────────────────────────
        try:
            frame.gsr = float(line)
            return frame
        except ValueError:
            return None

    def send_command(self, cmd: str):
        """Envía un comando al STM32 (termina en \\n)."""
        if self.serial and self.serial.is_open:
            try:
                self.serial.write((cmd.strip() + "\n").encode("utf-8"))
            except serial.SerialException as e:
                self.error_message = f"Error al enviar comando: {e}"

    def start_demo_mode(self):
        """Simula el comportamiento del STM32 con datos sintéticos."""
        import math, random
        self.connected = True
        self._running  = True

        def _demo():
            t = 0
            # Simular calibraciones ya hechas
            base_normal = 820
            base_lie    = 1200
            calib_pot   = 3000

            while self._running:
                noise = random.gauss(0, 15)
                spike = 200 * math.exp(-((t % 100 - 60) ** 2) / 80) if t % 100 > 50 else 0
                raw   = int(max(0, min(4095, base_normal + noise + spike
                                       + 40 * math.sin(t / 12))))

                # Simular filtro IIR (alpha=32/256 ≈ 0.125)
                filt = int(max(0, min(4095, raw * 0.125 + (raw - noise) * 0.875)))

                if filt < calib_pot:
                    r_skin = int(((4096 + 2 * filt) * 10000) / (calib_pot - filt))
                else:
                    r_skin = 0

                hr_raw = int(2048 + 400 * math.sin(t / 3) + random.gauss(0, 20))
                hr_raw = max(0, min(4095, hr_raw))
                bpm    = int(60 + 10 * math.sin(t / 50))

                # Probabilidad de mentira calculada "en el MCU"
                if base_lie != base_normal:
                    lie_pct = int(max(0, min(100,
                        (filt - base_normal) * 100 // (base_lie - base_normal))))
                else:
                    lie_pct = 0

                if lie_pct >= 70:
                    estado = "MENTIRA"
                elif lie_pct >= 40:
                    estado = "SOSPECHA"
                else:
                    estado = "NORMAL"

                frame = SensorFrame(
                    gsr=float(raw),
                    gsr_filt=float(filt),
                    r_skin=float(r_skin),
                    hr=float(hr_raw),
                    bpm=float(bpm),
                    lie_pct=lie_pct,
                    estado=estado,
                )
                try:
                    self.data_queue.put_nowait(frame)
                except queue.Full:
                    self.data_queue.get_nowait()
                    self.data_queue.put_nowait(frame)
                t += 1
                time.sleep(0.035)

        self._thread = threading.Thread(target=_demo, daemon=True)
        self._thread.start()