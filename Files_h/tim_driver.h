/**
 * @file    tim_driver.h
 * @brief   TIM Driver API for STM32F4xx
 *
 * Provides functions to initialize timers, configure time intervals
 * or frequencies, and control capture/compare channels for PWM.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @version 1.1
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

/* -----------------------------------------------------------------------
 * Constants
 * --------------------------------------------------------------------- */

/** System clock frequency in Hz (HSI default on STM32F411RE) */
#define TIM_SYSTEM_CLOCK_HZ     16000000UL

/* -----------------------------------------------------------------------
 * Type Definitions
 * --------------------------------------------------------------------- */

/**
 * @brief Return status codes for TIM driver functions.
 */
typedef enum
{
    TIM_OK      = 0,    /**< Operation successful              */
    TIM_ERROR   = 1,    /**< Generic error                     */
    TIM_INVALID = 2     /**< Invalid parameter                 */
} tim_status_t;

/**
 * @brief Capture/compare channel identifiers.
 */
typedef enum
{
    TIM_CHANNEL_1 = 1,
    TIM_CHANNEL_2 = 2,
    TIM_CHANNEL_3 = 3,
    TIM_CHANNEL_4 = 4
} tim_channel_t;

/**
 * @brief Compare/PWM modes for CCMRx register.
 */
typedef enum
{
    TIM_COMPARE_MODE_FROZEN    = 0x00,  /**< Output unchanged              */
    TIM_COMPARE_MODE_ACTIVE    = 0x01,  /**< Set active on match           */
    TIM_COMPARE_MODE_INACTIVE  = 0x02,  /**< Set inactive on match         */
    TIM_COMPARE_MODE_TOGGLE    = 0x03,  /**< Toggle on match               */
    TIM_COMPARE_MODE_PWM1      = 0x06,  /**< PWM mode 1 (high while < CCR) */
    TIM_COMPARE_MODE_PWM2      = 0x07   /**< PWM mode 2 (low while < CCR)  */
} tim_compare_mode_t;

/* -----------------------------------------------------------------------
 * Function Prototypes
 * --------------------------------------------------------------------- */

/**
 * @brief  Initializes the TIM subsystem.
 *         Disables clocks for all TIM peripherals (reset state).
 *         Call once at startup before any other TIM function.
 * @return TIM_OK on success.
 */
tim_status_t tim_init(void);

/**
 * @brief  Enables the peripheral clock and resets a specific TIM peripheral.
 * @param  tim  Pointer to a TIM peripheral (TIM1 … TIM5, TIM9 … TIM11).
 * @return TIM_OK on success, TIM_INVALID if tim is NULL or unknown.
 */
tim_status_t tim_initTimer(TIM_TypeDef *tim);

/**
 * @brief  Configures PSC and ARR to generate an update event every
 *         the specified number of milliseconds.
 * @param  tim     Pointer to the TIM peripheral.
 * @param  ms      Desired interval in milliseconds (1 … 65535).
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_setTimerMs(TIM_TypeDef *tim, uint32_t ms);

/**
 * @brief  Configures PSC and ARR to generate an update event at
 *         the specified frequency in Hz.
 * @param  tim   Pointer to the TIM peripheral.
 * @param  freq  Desired frequency in Hz (1 … TIM_SYSTEM_CLOCK_HZ).
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_setTimerFreq(TIM_TypeDef *tim, uint32_t freq);

/**
 * @brief  Enables and starts the specified timer (sets CEN bit in CR1).
 * @param  tim  Pointer to the TIM peripheral.
 * @return TIM_OK on success, TIM_INVALID if tim is NULL.
 */
tim_status_t tim_enableTimer(TIM_TypeDef *tim);

/**
 * @brief  Disables the specified timer (clears CEN bit in CR1).
 * @param  tim  Pointer to the TIM peripheral.
 * @return TIM_OK on success, TIM_INVALID if tim is NULL.
 */
tim_status_t tim_disableTimer(TIM_TypeDef *tim);

/**
 * @brief  Blocks execution until the timer update flag (UIF) is set,
 *         then clears the flag.
 * @param  tim  Pointer to the TIM peripheral.
 * @return TIM_OK on success, TIM_INVALID if tim is NULL.
 */
tim_status_t tim_waitTimer(TIM_TypeDef *tim);

/**
 * @brief  Sets the CCR value for a capture/compare channel.
 *         This defines the compare threshold (e.g. duty cycle for PWM).
 * @param  tim      Pointer to the TIM peripheral.
 * @param  channel  Channel number (TIM_CHANNEL_1 … TIM_CHANNEL_4).
 * @param  value    Compare value (0 … ARR).
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_setTimerCompareChannelValue(TIM_TypeDef *tim,
                                             tim_channel_t channel,
                                             uint32_t value);

/**
 * @brief  Configures the output compare mode for a channel via CCMRx.
 * @param  tim      Pointer to the TIM peripheral.
 * @param  channel  Channel number.
 * @param  mode     Compare mode (tim_compare_mode_t).
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_setTimerCompareMode(TIM_TypeDef *tim,
                                     tim_channel_t channel,
                                     tim_compare_mode_t mode);

/**
 * @brief  Enables a capture/compare channel output (sets CCxE in CCER).
 *         Also enables the output preload for PWM stability.
 * @param  tim      Pointer to the TIM peripheral.
 * @param  channel  Channel number.
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_enableTimerCompareChannel(TIM_TypeDef *tim,
                                           tim_channel_t channel);

/**
 * @brief  Disables a capture/compare channel output (clears CCxE in CCER).
 * @param  tim      Pointer to the TIM peripheral.
 * @param  channel  Channel number.
 * @return TIM_OK on success, TIM_INVALID on bad parameters.
 */
tim_status_t tim_disableTimerCompareChannel(TIM_TypeDef *tim,
                                            tim_channel_t channel);

#endif /* TIM_DRIVER_H */
