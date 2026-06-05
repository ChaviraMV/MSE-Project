/**
 * @file    lcd_ssd1306.h
 * @brief   Driver minimalista para OLED SSD1306 128×64 via I2C
 *          Orientado al Detector de Mentiras — STM32F411RE
 *
 * Muestra en pantalla:
 *   Línea 0 (grande):  LieP%  con barra de progreso
 *   Línea 1:           Estado  (NORMAL / SOSPECHA / MENTIRA / SIN_CAL)
 *   Línea 2:           BPM
 *   Línea 3:           R_piel en kΩ o MΩ
 *
 * Pines I2C1 (AF4):
 *   PB8 → SCL
 *   PB9 → SDA
 *
 * Dirección I2C del SSD1306: 0x3C (SA0 = GND) ó 0x3D (SA0 = VCC)
 *
 * Dependencias: i2c_driver.h, gpio_driver.h
 */

#ifndef LCD_SSD1306_H
#define LCD_SSD1306_H

#include <stdint.h>

/* ── Configuración ───────────────────────────────────────────────────── */
#define SSD1306_I2C_ADDR    0x3CU   /**< Cambiar a 0x3D si SA0=VCC      */
#define SSD1306_WIDTH       128U
#define SSD1306_HEIGHT      64U

/* ── API pública ─────────────────────────────────────────────────────── */

/**
 * @brief  Inicializa I2C1 (PB6/PB7) y el display SSD1306.
 *         Llamar una sola vez en main() antes del loop.
 */
void lcd_init(void);

/**
 * @brief  Actualiza toda la pantalla con los datos del detector.
 *
 * @param  lie_pct   Probabilidad de mentira 0–100, o 255 = sin calibrar.
 * @param  estado    String: "NORMAL", "SOSPECHA", "MENTIRA" o "SIN_CAL".
 * @param  bpm       Frecuencia cardíaca en BPM (0 = sin dedo).
 * @param  r_skin    Resistencia de piel en Ω.
 */
void lcd_update(uint8_t lie_pct, const char *estado,
                uint16_t bpm, uint32_t r_skin);

/**
 * @brief  Muestra la gráfica de señal GSR y el valor de resistencia.
 *
 *         Layout 128×64:
 *           Fila superior (8 px): "GSR  R:XXXkO"
 *           Área inferior (55 px): historial de barras verticales
 *
 *         Llama internamente a lcd_init() con la configuración de
 *         PB8 (SCL) y PB9 (SDA).  Puede llamarse cada ciclo del loop
 *         en lugar de lcd_update() cuando se desea ver la gráfica raw.
 *
 * @param  gsr_filt  Valor ADC filtrado 0–4095 de la señal GSR.
 * @param  r_skin    Resistencia de piel en Ω (para el encabezado).
 */
void lcd_update_gsr_graph(uint16_t gsr_filt, uint32_t r_skin);

#endif /* LCD_SSD1306_H */
