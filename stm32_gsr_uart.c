/* stm32_gsr_uart.c  —  Ejemplo de código STM32 (HAL)
 * Physiological Lie Detector · Grupo 6K · CETYS 2026
 *
 * Este fragmento va dentro de main.c (o en un módulo aparte).
 * Asume que configuraste en STM32CubeIDE:
 *   • ADC1 → Channel conectado al GSR (PA0 recomendado)
 *   • USART2 → 115200 baud, 8N1, TX=PA2 / RX=PA3
 *   • TIM6  → interrupción cada 50 ms (20 Hz)
 *
 * Protocolo de salida (una línea por muestra):
 *   GSR:2048\r\n
 *
 * Con múltiples sensores (futuro):
 *   GSR:2048,HR:75,TEMP:36.5\r\n
 *
 * Comandos recibidos desde Python:
 *   "CALIBRATE\n"  →  el MCU puede encender un LED de calibración, etc.
 *************************************************************************/

#include "main.h"
#include <stdio.h>
#include <string.h>

/* ── Variables externas generadas por CubeMX ────────────────────────── */
extern ADC_HandleTypeDef  hadc1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim6;

/* ── Buffers ─────────────────────────────────────────────────────────── */
#define TX_BUF_SIZE  64
#define RX_BUF_SIZE  32

static char    tx_buf[TX_BUF_SIZE];
static uint8_t rx_byte;                  // recepción byte a byte
static char    rx_line[RX_BUF_SIZE];
static uint8_t rx_idx = 0;
static volatile uint8_t send_flag = 0;  // 1 → enviar muestra

/* ── Prototipos ──────────────────────────────────────────────────────── */
static uint16_t read_adc(void);
static void     process_command(const char *cmd);

/* ── Llamar al final de MX_Init, antes del bucle while(1) ────────────── */
void LieDetector_Init(void)
{
    HAL_TIM_Base_Start_IT(&htim6);                         // inicia timer
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);             // recepción IT
}

/* ── Llamar dentro del while(1) de main ─────────────────────────────── */
void LieDetector_Task(void)
{
    if (!send_flag) return;
    send_flag = 0;

    uint16_t gsr = read_adc();

    /* Formato básico GSR */
    int len = snprintf(tx_buf, TX_BUF_SIZE, "GSR:%u\r\n", gsr);

    /*
     * Cuando agregues más sensores:
     * int len = snprintf(tx_buf, TX_BUF_SIZE,
     *     "GSR:%u,HR:%u,TEMP:%.1f\r\n", gsr, hr, temp);
     */

    HAL_UART_Transmit(&huart2, (uint8_t *)tx_buf, (uint16_t)len, 10);
}

/* ── Callback del Timer (cada 50 ms → 20 Hz) ────────────────────────── */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
        send_flag = 1;
}

/* ── Callback de recepción UART (byte a byte) ────────────────────────── */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '\n' || rx_idx >= RX_BUF_SIZE - 1)
        {
            rx_line[rx_idx] = '\0';
            process_command(rx_line);
            rx_idx = 0;
        }
        else if (rx_byte != '\r')
        {
            rx_line[rx_idx++] = (char)rx_byte;
        }
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);   // re-arm
    }
}

/* ── Leer ADC (modo polling, 12 bits) ───────────────────────────────── */
static uint16_t read_adc(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 5);
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* ── Procesar comandos de Python ─────────────────────────────────────── */
static void process_command(const char *cmd)
{
    if (strcmp(cmd, "CALIBRATE") == 0)
    {
        /* Ejemplo: encender LED azul durante calibración */
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        HAL_Delay(3000);
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    }
    /* Agregar más comandos aquí */
}

/*
 * ─── Configuración del Timer TIM6 en STM32CubeIDE (.ioc) ───────────────
 *
 *   Si el reloj APB1 = 84 MHz:
 *     Prescaler  = 8399   → fclk = 84M / (8399+1) = 10 000 Hz
 *     Period     =  499   → interrupción cada 500 ticks = 50 ms (20 Hz)
 *
 *   Si el reloj APB1 = 16 MHz (HSI sin PLL):
 *     Prescaler  = 1599
 *     Period     =  499
 *
 *  Recuerda habilitar la interrupción global de TIM6 en NVIC.
 * ────────────────────────────────────────────────────────────────────── */
