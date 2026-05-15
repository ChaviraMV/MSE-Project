"""
uart_handler.py — Módulo de comunicación UART con STM32
Physiological Lie Detector · Grupo 6K · CETYS Universidad 2026

Protocolo del firmware LAB4_GSR (main.c):

  Modo calibración (GSR_CALIBRATION == 0):
      [CAL] Raw: 312\n

  Modo normal (GSR_CALIBRATION != 0):
      Raw: 820 | Filtrado: 815 | R_piel: 24500 ohm | Estado: NORMAL\n

  Formatos futuros (multi-sensor):
      GSR:2048,HR:75,TEMP:36.5\n
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
    """Un frame de datos recibido del STM32."""
    timestamp: float        = field(default_factory=time.time)
    gsr:       Optional[float] = None   # valor raw ADC (0-4095)
    gsr_filt:  Optional[float] = None   # valor filtrado (moving average LAB4)
    r_skin:    Optional[float] = None   # resistencia de piel en ohm
    estado:    Optional[str]   = None   # "NORMAL" | "ACTIVO"
    hr:        Optional[float] = None   # futuro: frecuencia cardiaca
    temp:      Optional[float] = None   # futuro: temperatura
    cal_mode:  bool            = False  # True si la linea es [CAL]
    raw:       str             = ""


class UARTHandler:
    """
    Maneja la conexión serial con el STM32 en un hilo separado.
    Los datos se encolan en self.data_queue para que la GUI los consuma.
    """

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

    # ── Descubrimiento de puertos ──────────────────────────────────────────
    @staticmethod
    def list_ports() -> list[str]:
        """Devuelve lista de puertos COM/tty disponibles."""
        ports = serial.tools.list_ports.comports()
        return [p.device for p in sorted(ports)]

    @staticmethod
    def find_stm32_port() -> Optional[str]:
        """Intenta encontrar automáticamente un STM32 conectado."""
        STM32_VIDS = {0x0483}  # STMicroelectronics VID
        for p in serial.tools.list_ports.comports():
            if p.vid in STM32_VIDS:
                return p.device
        return None

    # ── Conexión / desconexión ─────────────────────────────────────────────
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

    # ── Hilo lector ───────────────────────────────────────────────────────
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
                            # Descarta datos si la cola está llena (no bloquea GUI)
                            try:
                                self.data_queue.put_nowait(frame)
                            except queue.Full:
                                self.data_queue.get_nowait()  # descarta el más viejo
                                self.data_queue.put_nowait(frame)
            except serial.SerialException:
                self.connected = False
                self._running = False
                self.error_message = "Conexión serial perdida."
                break
            except Exception:
                pass

    # ── Parser de protocolo ───────────────────────────────────────────────
    @staticmethod
    def _parse_line(line: str) -> Optional["SensorFrame"]:
        """
        Parsea una línea del firmware LAB4_GSR.

        Formatos soportados:

        1) Modo calibración (GSR_CALIBRATION == 0 en main.c):
               [CAL] Raw: 312

        2) Modo normal (GSR_CALIBRATION != 0 en main.c):
               Raw: 820 | Filtrado: 815 | R_piel: 24500 ohm | Estado: NORMAL

        3) Formato futuro multi-sensor:
               GSR:2048,HR:75,TEMP:36.5

        4) Valor crudo numérico (fallback):
               2048
        """
        if not line:
            return None

        frame = SensorFrame(raw=line)

        # ── Formato 1: [CAL] Raw: 312 ─────────────────────────────────────
        if line.startswith("[CAL]"):
            frame.cal_mode = True
            # Extraer el número después de "Raw:"
            try:
                raw_str = line.split("Raw:")[-1].strip()
                frame.gsr = float(raw_str.split()[0])
            except (IndexError, ValueError):
                pass
            return frame if frame.gsr is not None else None

        # ── Formato 2: Raw: 820 | Filtrado: 815 | R_piel: 24500 ohm | Estado: NORMAL
        if "Filtrado:" in line or "R_piel:" in line:
            # Dividir por "|" y parsear cada campo
            parts = [p.strip() for p in line.split("|")]
            for part in parts:
                if part.startswith("Raw:"):
                    try:
                        frame.gsr = float(part.split(":", 1)[1].strip())
                    except (IndexError, ValueError):
                        pass
                elif part.startswith("Filtrado:"):
                    try:
                        frame.gsr_filt = float(part.split(":", 1)[1].strip())
                    except (IndexError, ValueError):
                        pass
                elif part.startswith("R_piel:"):
                    # "24500 ohm" → tomar solo el número
                    try:
                        val_str = part.split(":", 1)[1].strip().split()[0]
                        frame.r_skin = float(val_str)
                    except (IndexError, ValueError):
                        pass
                elif part.startswith("Estado:"):
                    frame.estado = part.split(":", 1)[1].strip()

            return frame if frame.gsr is not None else None

        # ── Formato 3: GSR:2048,HR:75,TEMP:36.5 ──────────────────────────
        if ":" in line:
            parts = line.split(",")
            for part in parts:
                part = part.strip()
                if ":" not in part:
                    continue
                key, _, val = part.partition(":")
                key = key.strip().upper()
                try:
                    fval = float(val.strip())
                    if key == "GSR":
                        frame.gsr = fval
                    elif key == "HR":
                        frame.hr = fval
                    elif key in ("TEMP", "TMP"):
                        frame.temp = fval
                except ValueError:
                    pass
            if frame.gsr is not None or frame.hr is not None:
                return frame
            return None

        # ── Formato 4: valor crudo numérico ───────────────────────────────
        try:
            frame.gsr = float(line)
            return frame
        except ValueError:
            return None

    # ── Envío de comandos al STM32 ────────────────────────────────────────
    def send_command(self, cmd: str):
        """Envía un comando ASCII al STM32 (ej: 'CALIBRATE\n')."""
        if self.serial and self.serial.is_open:
            try:
                self.serial.write((cmd.strip() + "\n").encode("utf-8"))
            except serial.SerialException as e:
                self.error_message = f"Error al enviar comando: {e}"

    # ── Simulador (modo demo sin hardware) ───────────────────────────────
    def start_demo_mode(self):
        """Genera datos que imitan el formato LAB4_GSR para probar la GUI sin STM32."""
        import math, random
        self.connected = True
        self._running = True

        def _demo():
            t = 0
            baseline = 820
            calib = 3000  # simula GSR_CALIBRATION del firmware
            while self._running:
                noise = random.gauss(0, 15)
                spike = 200 * math.exp(-((t % 100 - 60) ** 2) / 80) if t % 100 > 50 else 0
                raw   = int(max(0, min(4095, baseline + noise + spike + 40 * math.sin(t / 12))))
                filt  = int(max(0, min(4095, raw + random.gauss(0, 5))))

                # Fórmula Seeed Wiki adaptada
                if filt < calib:
                    r_skin = int(((4096 + 2 * filt) * 10000) / (calib - filt))
                else:
                    r_skin = 0

                estado = "ACTIVO" if r_skin < 10000 and r_skin > 0 else "NORMAL"

                frame = SensorFrame(
                    gsr=float(raw),
                    gsr_filt=float(filt),
                    r_skin=float(r_skin),
                    estado=estado,
                )
                try:
                    self.data_queue.put_nowait(frame)
                except queue.Full:
                    self.data_queue.get_nowait()
                    self.data_queue.put_nowait(frame)
                t += 1
                time.sleep(0.2)  # 5 Hz, igual que el firmware (PERIOD_MS = 200)

        self._thread = threading.Thread(target=_demo, daemon=True)
        self._thread.start()
