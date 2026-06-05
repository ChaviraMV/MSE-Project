/******************************************************************************
 * Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
 *****************************************************************************/
/**
 * @file uart.c
 * @brief UART Driver — TX + RX para STM32F411RE
 *
 * Recepción por INTERRUPCIÓN (RXNEIE) en lugar de polling.
 * Cada byte recibido se captura en USART2_IRQHandler sin importar
 * si el loop principal está ocupado transmitiendo.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @date    05/06/2026
 */

/*** Includes ***/
#include "uart.h"
#include "gpio_driver.h"

/*** Definiciones locales ***/
#define UART_TX_PORT    GPIOA
#define UART_TX_PIN     2U
#define UART_TX_AF      7U

#define UART_RX_PORT    GPIOA
#define UART_RX_PIN     3U
#define UART_RX_AF      7U

/* ── Buffer circular RX ─────────────────────────────────────────────── */
static volatile uint8_t s_rxBuf[UART_RX_BUF_SIZE];
static volatile uint8_t s_rxHead = 0U;   /* escritura — modificado en ISR  */
static volatile uint8_t s_rxTail = 0U;   /* lectura   — modificado en main */

/*** uart_init **************************************************************/
uart_status_t uart_init(void)
{
    /* ── TX: PA2 AF7 ─────────────────────────────────────────────────── */
    gpio_initPort(UART_TX_PORT);
    gpio_setAlternateFunction(UART_TX_PORT, UART_TX_PIN, UART_TX_AF);

    /* ── RX: PA3 AF7 ─────────────────────────────────────────────────── */
    gpio_initPort(UART_RX_PORT);
    gpio_setAlternateFunction(UART_RX_PORT, UART_RX_PIN, UART_RX_AF);

    /* ── USART2 clock ────────────────────────────────────────────────── */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* ── BRR ─────────────────────────────────────────────────────────── */
    USART2->BRR = (uint32_t)(UART_SYSTEM_CLOCK_HZ / UART_BAUD_RATE);

    /* ── Habilitar TX + RX + interrupción RXNE + USART ─────────────── */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* ── Habilitar IRQ USART2 en el NVIC (prioridad 1) ──────────────── */
    NVIC_SetPriority(USART2_IRQn, 1U);
    NVIC_EnableIRQ(USART2_IRQn);

    return UART_OK;
}

/*** USART2_IRQHandler ******************************************************/
/**
 * Se ejecuta automáticamente cada vez que llega un byte por RX,
 * sin importar lo que esté haciendo el loop principal.
 * Almacena el byte en el buffer circular; si está lleno lo descarta.
 */
void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_RXNE)
    {
        uint8_t byte = (uint8_t)(USART2->DR & 0xFFU);
        uint8_t next = (uint8_t)((s_rxHead + 1U) % UART_RX_BUF_SIZE);
        if (next != s_rxTail)       /* hay espacio en el buffer */
        {
            s_rxBuf[s_rxHead] = byte;
            s_rxHead = next;
        }
        /* Si el buffer está lleno, descartamos el byte — nunca bloqueamos */
    }
}

/*** uart_poll_rx ***********************************************************/
/**
 * Mantenida por compatibilidad con las llamadas en main.c.
 * La recepción real ya ocurre en la ISR; esta función no hace nada.
 */
void uart_poll_rx(void)
{
    /* La ISR USART2_IRQHandler captura los bytes automáticamente. */
}

/*** uart_write *************************************************************/
uart_status_t uart_write(uint8_t data)
{
    while (!(USART2->SR & USART_SR_TXE))
    {
        /* busy-wait hasta que el registro de transmisión esté libre */
    }
    USART2->DR = (uint32_t)(data & 0xFFU);
    return UART_OK;
}

/*** uart_read **************************************************************/
uart_status_t uart_read(uint8_t *out)
{
    if (out == NULL)           { return UART_INVALID; }
    if (s_rxHead == s_rxTail) { return UART_ERROR;   }  /* buffer vacío */

    *out    = s_rxBuf[s_rxTail];
    s_rxTail = (uint8_t)((s_rxTail + 1U) % UART_RX_BUF_SIZE);
    return UART_OK;
}

/*** uart_read_line *********************************************************/
/**
 * Extrae del buffer circular todos los bytes hasta encontrar '\n'.
 * Si no hay '\n' aún, devuelve UART_ERROR sin consumir nada.
 * Seguro frente a la ISR: s_rxHead es volatile y se lee una sola vez.
 */
uart_status_t uart_read_line(char *buf, uint8_t size)
{
    if (buf == NULL || size == 0U) { return UART_INVALID; }

    /* Capturar head una sola vez para evitar race condition con la ISR */
    uint8_t head  = s_rxHead;
    uint8_t idx   = s_rxTail;
    uint8_t count = 0U;
    uint8_t found = 0U;

    /* Buscar '\n' sin consumir bytes */
    while (idx != head)
    {
        if (s_rxBuf[idx] == (uint8_t)'\n')
        {
            found = 1U;
            break;
        }
        idx = (uint8_t)((idx + 1U) % UART_RX_BUF_SIZE);
        count++;
        if (count >= size) { break; }   /* prevenir desbordamiento */
    }

    if (!found) { return UART_ERROR; }  /* línea incompleta todavía */

    /* Consumir y copiar hasta '\n' */
    uint8_t dst = 0U;
    while (s_rxTail != head)
    {
        uint8_t byte = s_rxBuf[s_rxTail];
        s_rxTail = (uint8_t)((s_rxTail + 1U) % UART_RX_BUF_SIZE);
        if (byte == (uint8_t)'\n') { break; }
        if (dst < (uint8_t)(size - 1U))
        {
            buf[dst++] = (char)byte;
        }
    }
    buf[dst] = '\0';
    return UART_OK;
}