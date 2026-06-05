/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *****************************************************************************/
/**
 * @file uart.h
 * @brief UART Driver for STM32F411RE — TX + RX
 *
 * Configura USART2 para comunicación bidireccional a 115200 baud.
 * TX: PA2 (AF7) — envío de telemetría a Python
 * RX: PA3 (AF7) — recepción de comandos desde Python
 *
 * Comandos reconocidos (terminan en '\n'):
 *   "CAL_NORMAL\n"  → iniciar calibración en reposo (preguntas normales)
 *   "CAL_LIE\n"     → iniciar calibración de mentira (preguntas conocidas falsas)
 *   "STOP_CAL\n"    → abortar calibración en curso
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date    05/06/2026
 */

#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include "stm32f4xx.h"

/* ── Configuración ────────────────────────────────────────────────── */
#define UART_SYSTEM_CLOCK_HZ    16000000UL
#define UART_BAUD_RATE          115200UL
#define UART_RX_BUF_SIZE        64U     /**< Buffer circular de recepción */

/* ── Tipos ────────────────────────────────────────────────────────── */
typedef enum {
    UART_OK      = 0,
    UART_ERROR   = 1,
    UART_INVALID = 2
} uart_status_t;

/* ── Prototipos ───────────────────────────────────────────────────── */

/**
 * @brief  Inicializa USART2 en modo TX+RX a 115200 baud.
 *         PA2 = TX (AF7), PA3 = RX (AF7).
 */
uart_status_t uart_init(void);

/**
 * @brief  Transmite un byte por USART2 (polling TXE).
 */
uart_status_t uart_write(uint8_t data);

/**
 * @brief  Lee un byte del buffer circular de recepción.
 *         Debe llamarse desde el loop principal o desde ISR USART2.
 * @param  out  Byte leído (válido solo si retorna UART_OK).
 * @return UART_OK si había dato, UART_ERROR si buffer vacío.
 */
uart_status_t uart_read(uint8_t *out);

/**
 * @brief  Sondea RXNE y almacena el byte en el buffer circular.
 *         Llamar periódicamente desde el loop principal (no usa ISR).
 */
void uart_poll_rx(void);

/**
 * @brief  Lee una línea completa (hasta '\n') del buffer RX.
 * @param  buf   Destino (se escribe sin el '\n', con '\0' al final).
 * @param  size  Tamaño máximo del buffer destino.
 * @return UART_OK si se obtuvo una línea completa; UART_ERROR si no.
 */
uart_status_t uart_read_line(char *buf, uint8_t size);

#endif /* __UART_H__ */
