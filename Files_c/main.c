/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright.
 *
 *****************************************************************************/
/**
 * @file main.c
 * @brief Detector Fisiológico de Mentiras — STM32F411RE (Nucleo)
 *
 * ── Fórmula resistencia (Grove GSR, ADC 12-bit) ───────────────────────
 *   R_piel = ((4096 + 2 × ADC_filt) × 10000) / (CAL - ADC_filt)   [Ω]
 *
 * ── Lógica de mentira ─────────────────────────────────────────────────
 *   Estrés/mentira → más sudor → más conductancia → R_piel BAJA
 *   Reposo/normal  → menos sudor                  → R_piel ALTA
 *   base_normal (Ω) > base_lie (Ω)
 *
 * ── Protocolo UART TX ─────────────────────────────────────────────────
 *   Modo pot:    [CAL] GSR_Raw: %d | HR_Raw: %d
 *   Normal:      Raw:%d|Filt:%d|Rpiel:%d|HR:%d|BPM:%d|LieP:%d|Estado:%s
 *   Cal normal:  [CALN] GSR_Raw: %d | HR_Raw: %d | Rpiel: %d | Progress: %d/%d
 *   Cal mentira: [CALL] GSR_Raw: %d | HR_Raw: %d | Rpiel: %d | Progress: %d/%d
 *   Fin cal:     [CALN_DONE] BaseNorm:%d  /  [CALL_DONE] BaseLie:%d
 *
 * ── Protocolo UART RX ─────────────────────────────────────────────────
 *   "CAL_NORMAL\n"  "CAL_LIE\n"  "STOP_CAL\n"
 *
 * ── Hardware (Nucleo-F411RE) ──────────────────────────────────────────
 *   PA0→ADC GSR  PA1→ADC HR  PA2→TX  PA3→RX  PB4→PWM GSR  PB5→LED beat
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date    05/06/2026
 */

/*** Includes ***/
#include "gpio_driver.h"
#include "adc_driver.h"
#include "tim_driver.h"
#include "sensor.h"
#include "serial.h"
#include "uart.h"
#include "utils.h"
#include "lcd_ssd1306.h"    /* OLED SSD1306 — PB8(SCL) PB9(SDA) */

/* ═══════════════════════════════════════════════════════════════════════════
 * CONSTANTES CONFIGURABLES
 * ═══════════════════════════════════════════════════════════════════════════ */

#define VREF_MV             3300U
#define ADC_FULL_SCALE      4095U
#define PERIOD_MS           35U

#define GSR_IIR_ALPHA       32U
#define HR_IIR_ALPHA        128U

#define GSR_CALIBRATION     2217U
#define GSR_R_REF_OHM       10000U

#define CALIB_SAMPLES       150U

#define DISPLAY_TIM         TIM3
#define DISPLAY_TIM_PERIOD  4095U
#define DISPLAY_TIM_PSC     3U
#define BEAT_LED_PORT       GPIOB
#define BEAT_LED_PIN        GPIO_PIN_5

#define CMD_CAL_NORMAL      "CAL_NORMAL"
#define CMD_CAL_LIE         "CAL_LIE"
#define CMD_STOP_CAL        "STOP_CAL"

/* ═══════════════════════════════════════════════════════════════════════════
 * TIPOS INTERNOS
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    CAL_IDLE   = 0,
    CAL_NORMAL = 1,
    CAL_LIE    = 2
} CalibState_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * VARIABLES DE ESTADO
 * ═══════════════════════════════════════════════════════════════════════════ */

static uint32_t s_gsrFilt_x256    = 0U;
static uint32_t s_hrFilt_x256     = 0U;
static uint32_t s_hrSlowFilt_x256 = 0U;

static uint32_t s_tick           = 0U;
static uint32_t s_lastBeat       = 0U;
static uint16_t s_bpm            = 0U;
static uint8_t  s_pulseDetected  = 0U;
static uint32_t s_bpmHistory[3]  = {0U, 0U, 0U};
static uint8_t  s_bpmHistIdx     = 0U;

static CalibState_t s_calState   = CAL_IDLE;
static uint32_t     s_calAccum   = 0U;
static uint16_t     s_calCount   = 0U;

/* Bases calibradas en RESISTENCIA (Ω).
 * base_normal → R alta (reposo)   base_lie → R baja (estrés)          */
static uint32_t s_baseNormal = 0U;
static uint32_t s_baseLie    = 0U;

/* ═══════════════════════════════════════════════════════════════════════════
 * FUNCIONES ESTÁTICAS
 * ═══════════════════════════════════════════════════════════════════════════ */

static uint8_t str_eq(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (*a != *b) { return 0U; }
        a++; b++;
    }
    return (*a == '\0' && *b == '\0') ? 1U : 0U;
}

static uint16_t iir_filter(uint32_t *accum_x256, uint16_t new_sample, uint8_t alpha)
{
    uint32_t prev         = *accum_x256;
    uint32_t contrib_new  = (uint32_t)alpha * (uint32_t)new_sample;
    uint32_t contrib_prev = (uint32_t)(256U - alpha) * (prev >> 8U);
    *accum_x256 = contrib_new + contrib_prev;
    return (uint16_t)(*accum_x256 >> 8U);
}

/* ── Probabilidad de mentira basada en R_piel ────────────────────────
 * R alta = relajado = 0%    R baja = estresado = 100%
 * base_normal (R alta, reposo) > base_lie (R baja, estrés)            */
static uint8_t compute_lie_probability(uint32_t r_skin)
{
    if (s_baseNormal == 0U || s_baseLie == 0U) { return 255U; }

    uint32_t r_normal = s_baseNormal;
    uint32_t r_lie    = s_baseLie;

    /* Si el usuario calibró al revés, corregimos */
    if (r_lie > r_normal)
    {
        uint32_t tmp = r_normal;
        r_normal = r_lie;
        r_lie    = tmp;
    }

    if (r_normal == r_lie) { return 0U; }

    if (r_skin >= r_normal) { return 0U;   }  /* muy relajado  */
    if (r_skin <= r_lie)    { return 100U; }  /* muy estresado */

    /* LieP% = (r_normal - r_skin) / (r_normal - r_lie) × 100 */
    uint32_t rango = r_normal - r_lie;
    uint32_t caida = r_normal - r_skin;
    return (uint8_t)((caida * 100U) / rango);
}

static void display_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOB->MODER &= ~(0x3U << (4U * 2U));
    GPIOB->MODER |=  (0x2U << (4U * 2U));
    GPIOB->AFR[0] &= ~(0xFU << (4U * 4U));
    GPIOB->AFR[0] |=  (0x2U << (4U * 4U));

    TIM3->PSC   = DISPLAY_TIM_PSC;
    TIM3->ARR   = DISPLAY_TIM_PERIOD;
    TIM3->CCR1  = 0U;
    TIM3->CCMR1 = (6U << 4U) | (1U << 3U);
    TIM3->CCER  = TIM_CCER_CC1E;
    TIM3->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;

    GPIOB->MODER &= ~(0x3U << (5U * 2U));
    GPIOB->MODER |=  (0x1U << (5U * 2U));
    GPIOB->ODR   &= ~(1U << 5U);
}

static void display_update_gsr(uint16_t gsr_filt)
{
    TIM3->CCR1 = (uint32_t)gsr_filt;
}

static void display_beat_pulse(void)
{
    GPIOB->ODR |=  (1U << 5U);
    volatile uint32_t d = 8000U;
    while (d--) { __NOP(); }
    GPIOB->ODR &= ~(1U << 5U);
}

static void process_rx_command(const char *cmd)
{
    if (str_eq(cmd, CMD_CAL_NORMAL))
    {
        s_calState = CAL_NORMAL;
        s_calAccum = 0U;
        s_calCount = 0U;
        serial_printf("[INFO] Calibracion NORMAL iniciada (%d muestras)\n",
                      (int32_t)CALIB_SAMPLES);
    }
    else if (str_eq(cmd, CMD_CAL_LIE))
    {
        s_calState = CAL_LIE;
        s_calAccum = 0U;
        s_calCount = 0U;
        serial_printf("[INFO] Calibracion MENTIRA iniciada (%d muestras)\n",
                      (int32_t)CALIB_SAMPLES);
    }
    else if (str_eq(cmd, CMD_STOP_CAL))
    {
        s_calState = CAL_IDLE;
        serial_printf("[INFO] Calibracion abortada\n");
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    gpio_init();
    serial_init();

    tim_init();
    tim_initTimer(TIM2);
    tim_setTimerMs(TIM2, PERIOD_MS);
    tim_enableTimer(TIM2);

    Sensor_Config_t gsr_cfg = {
        .adcInstance = SENSOR_ADC_INSTANCE,
        .adcChannel  = SENSOR_ADC_CHANNEL,
        .sampleTime  = SENSOR_SAMPLE_TIME
    };
    sensor_init(&gsr_cfg);

    {
        uint16_t first = 0U;
        sensor_startConversion();
        sensor_readValue(&first);
        s_gsrFilt_x256 = (uint32_t)first << 8U;
    }

    Sensor_Config_t hr_cfg = {
        .adcInstance = HR_ADC_INSTANCE,
        .adcChannel  = HR_ADC_CHANNEL,
        .sampleTime  = HR_SAMPLE_TIME
    };

    display_init();
    lcd_init();     /* OLED SSD1306: I2C1 en PB8(SCL) / PB9(SDA) */

#if (GSR_CALIBRATION == 0U)
    serial_printf("=== MODO CALIBRACION POTENCIOMETRO ===\n");
    serial_printf("Sin electrodos. Gira pot hasta minimizar Raw.\n\n");
#endif

    char cmd_buf[UART_RX_BUF_SIZE];

    while (1)
    {
        tim_waitTimer(TIM2);
        s_tick++;

        /* ── RX ─────────────────────────────────────────────────────── */
        uart_poll_rx();
        if (uart_read_line(cmd_buf, (uint8_t)sizeof(cmd_buf)) == UART_OK)
        {
            process_rx_command(cmd_buf);
        }

        /* ── GSR ─────────────────────────────────────────────────────── */
        sensor_init(&gsr_cfg);
        sensor_startConversion();
        uint16_t gsr_raw = 0U;
        sensor_readValue(&gsr_raw);

        uint16_t gsr_filt = iir_filter(&s_gsrFilt_x256, gsr_raw, GSR_IIR_ALPHA);

        /* ── HR ──────────────────────────────────────────────────────── */
        sensor_init(&hr_cfg);
        sensor_startConversion();
        uint16_t hr_raw = 0U;
        sensor_readValue(&hr_raw);

        uint16_t hr_filt = iir_filter(&s_hrFilt_x256,     hr_raw, HR_IIR_ALPHA);
        uint16_t hr_base = iir_filter(&s_hrSlowFilt_x256, hr_raw, 4U);

        /* ── Detección de latido ─────────────────────────────────────── */
        uint8_t beat_now     = 0U;
        int16_t signal_delta = (int16_t)hr_filt - (int16_t)hr_base;
        uint8_t dedo_presente = (hr_filt > 1000U && hr_filt < 4090U) ? 1U : 0U;
        uint8_t refractory    = ((s_tick - s_lastBeat) < 12U) ? 1U : 0U;

        if (dedo_presente && !refractory && signal_delta > 40)
        {
            if (s_pulseDetected == 0U)
            {
                s_pulseDetected = 1U;
                uint32_t interval_ms = (s_tick - s_lastBeat) * PERIOD_MS;
                if (interval_ms > 300U && interval_ms < 2000U)
                {
                    s_bpmHistory[s_bpmHistIdx] = 60000U / interval_ms;
                    s_bpmHistIdx = (s_bpmHistIdx + 1U) % 3U;

                    uint32_t bpm_sum = 0U;
                    uint8_t  bpm_cnt = 0U;
                    for (uint8_t i = 0U; i < 3U; i++) {
                        if (s_bpmHistory[i] > 0U) {
                            bpm_sum += s_bpmHistory[i];
                            bpm_cnt++;
                        }
                    }
                    s_bpm = (bpm_cnt > 0U) ? (uint16_t)(bpm_sum / bpm_cnt) : 0U;
                }
                s_lastBeat = s_tick;
                beat_now   = 1U;
            }
        }
        else if (signal_delta < 0)
        {
            s_pulseDetected = 0U;
        }

        if ((s_tick - s_lastBeat) > 86U || !dedo_presente)
        {
            s_bpm           = 0U;
            s_bpmHistory[0] = 0U;
            s_bpmHistory[1] = 0U;
            s_bpmHistory[2] = 0U;
            s_bpmHistIdx    = 0U;
        }

        if (beat_now) { display_beat_pulse(); }
        display_update_gsr(gsr_filt);

#if (GSR_CALIBRATION == 0U)
        serial_printf("[CAL] GSR_Raw: %d | HR_Raw: %d\n",
                      (int32_t)gsr_raw, (int32_t)hr_raw);
#else
        /* ── Calcular R_piel SIEMPRE (antes de calibración y frame normal)
         * Fórmula Grove GSR adaptada a 12 bits:
         * R = ((4096 + 2 × ADC_filt) × 10000) / (CAL - ADC_filt)         */
        uint32_t r_skin_ohm = 0U;
        if (gsr_filt < (uint16_t)GSR_CALIBRATION)
        {
            uint32_t num = ((uint32_t)4096U + 2U * (uint32_t)gsr_filt)
                           * (uint32_t)GSR_R_REF_OHM;
            uint32_t den = (uint32_t)GSR_CALIBRATION - (uint32_t)gsr_filt;
            r_skin_ohm   = num / den;
        }
        else
        {
            r_skin_ohm = 1000000U;  /* sin dedo o saturado */
        }

        /* ── Calibración fisiológica ─────────────────────────────────── */
        if (s_calState == CAL_NORMAL || s_calState == CAL_LIE)
        {
            /* Acumular RESISTENCIA (Ω), no ADC */
            s_calAccum += r_skin_ohm;
            s_calCount++;

            const char *tag = (s_calState == CAL_NORMAL) ? "[CALN]" : "[CALL]";

            /* Incluir Rpiel en el mensaje para que Python pueda graficarlo */
            serial_printf("%s GSR_Raw: %d | HR_Raw: %d | Rpiel: %d | Progress: %d/%d\n",
                          tag,
                          (int32_t)gsr_raw,
                          (int32_t)hr_raw,
                          (int32_t)r_skin_ohm,
                          (int32_t)s_calCount,
                          (int32_t)CALIB_SAMPLES);

            /* ── OLED: seguir mostrando gráfica GSR durante calibración ─ */
            lcd_update_gsr_graph(gsr_filt, r_skin_ohm);

            if (s_calCount >= CALIB_SAMPLES)
            {
                uint32_t avg = s_calAccum / (uint32_t)s_calCount;
                if (s_calState == CAL_NORMAL)
                {
                    s_baseNormal = avg;
                    serial_printf("[CALN_DONE] BaseNorm:%d\n", (int32_t)s_baseNormal);
                }
                else
                {
                    s_baseLie = avg;
                    serial_printf("[CALL_DONE] BaseLie:%d\n", (int32_t)s_baseLie);
                }
                s_calState = CAL_IDLE;
                s_calAccum = 0U;
                s_calCount = 0U;
            }
        }
        else
        {
            /* ── Frame normal ────────────────────────────────────────── */
            /* LieP calculado sobre R_piel: R baja → mayor estrés → más % */
            uint8_t lie_pct = compute_lie_probability(r_skin_ohm);

            const char *estado;
            if      (lie_pct == 255U) { estado = "SIN_CAL";  }
            else if (lie_pct >= 70U)  { estado = "MENTIRA";  }
            else if (lie_pct >= 40U)  { estado = "SOSPECHA"; }
            else                      { estado = "NORMAL";   }

            serial_printf(
                "Raw:%d|Filt:%d|Rpiel:%d|HR:%d|BPM:%d|LieP:%d|Estado:%s\n",
                (int32_t)gsr_raw,
                (int32_t)gsr_filt,
                (int32_t)r_skin_ohm,
                (int32_t)hr_filt,
                (int32_t)s_bpm,
                (int32_t)(lie_pct == 255U ? -1 : (int32_t)lie_pct),
                estado
            );

            /* ── OLED: gráfica GSR + datos del detector ──────────────── */
            lcd_update_gsr_graph(gsr_filt, r_skin_ohm);
        }
#endif
    }

    return 0;
}