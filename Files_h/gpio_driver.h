/**
 * @file    gpio_driver.h
 * @brief   GPIO Driver API for STM32F4xx
 *
 * Provides functions to initialize ports, configure pin modes,
 * and control/read pin states.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @version 1.0
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"
#include <stddef.h> 

/* -----------------------------------------------------------------------
 * Type Definitions
 * --------------------------------------------------------------------- */

/**
 * @brief Return status codes for GPIO driver functions.
 */
typedef enum
{
    GPIO_OK      = 0,   /**< Operation successful              */
    GPIO_ERROR   = 1,   /**< Generic error                     */
    GPIO_INVALID = 2    /**< Invalid parameter (port/pin/mode) */
} gpio_status_t;

/**
 * @brief GPIO pin mode options.
 */
typedef enum
{
    GPIO_MODE_INPUT    = 0x00,  /**< Digital input                 */
    GPIO_MODE_OUTPUT   = 0x01,  /**< Digital output (push-pull)    */
    GPIO_MODE_ALT_FUNC = 0x02,  /**< Alternate function            */
    GPIO_MODE_ANALOG   = 0x03   /**< Analog mode                   */
} gpio_mode_t;

/**
 * @brief GPIO pin identifiers (0–15).
 */
typedef enum
{
    GPIO_PIN_0  = 0,
    GPIO_PIN_1  = 1,
    GPIO_PIN_2  = 2,
    GPIO_PIN_3  = 3,
    GPIO_PIN_4  = 4,
    GPIO_PIN_5  = 5,
    GPIO_PIN_6  = 6,
    GPIO_PIN_7  = 7,
    GPIO_PIN_8  = 8,
    GPIO_PIN_9  = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15
} gpio_pin_t;

/**
 * @brief GPIO pin digital state.
 */
typedef enum
{
    GPIO_PIN_LOW  = 0,
    GPIO_PIN_HIGH = 1
} gpio_pin_state_t;

/* -----------------------------------------------------------------------
 * Function Prototypes
 * --------------------------------------------------------------------- */

/**
 * @brief  Initializes the GPIO subsystem (resets all ports to default).
 *         Call once at startup before any other GPIO function.
 * @return GPIO_OK on success.
 */
gpio_status_t gpio_init(void);

/**
 * @brief  Enables the peripheral clock for a specific GPIO port.
 * @param  port  Pointer to the GPIO port (GPIOA … GPIOE).
 * @return GPIO_OK on success, GPIO_INVALID if port is NULL or unknown.
 */
gpio_status_t gpio_initPort(GPIO_TypeDef *port);

/**
 * @brief  Configures the mode of a specific pin.
 * @param  port  Pointer to the GPIO port.
 * @param  pin   Pin number (GPIO_PIN_0 … GPIO_PIN_15).
 * @param  mode  Desired pin mode (gpio_mode_t).
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_setPinMode(GPIO_TypeDef *port, gpio_pin_t pin, gpio_mode_t mode);

/**
 * @brief  Drives a pin to logic HIGH using BSRR.
 * @param  port  Pointer to the GPIO port.
 * @param  pin   Pin number.
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_setPin(GPIO_TypeDef *port, gpio_pin_t pin);

/**
 * @brief  Drives a pin to logic LOW using BSRR.
 * @param  port  Pointer to the GPIO port.
 * @param  pin   Pin number.
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_clearPin(GPIO_TypeDef *port, gpio_pin_t pin);

/**
 * @brief  Toggles the current state of a pin via ODR.
 * @param  port  Pointer to the GPIO port.
 * @param  pin   Pin number.
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_togglePin(GPIO_TypeDef *port, gpio_pin_t pin);

/**
 * @brief  Reads the current digital state of a pin via IDR.
 * @param  port   Pointer to the GPIO port.
 * @param  pin    Pin number.
 * @param  state  Output parameter; set to GPIO_PIN_LOW or GPIO_PIN_HIGH.
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_readPin(GPIO_TypeDef *port, gpio_pin_t pin, gpio_pin_state_t *state);

/**
 * @brief  Configures the alternate function for a specific pin.
 * @param  port     Pointer to the GPIO port.
 * @param  pin      Pin number.
 * @param  alt_func Alternate function number (0–15, see datasheet).
 * @return GPIO_OK on success, GPIO_INVALID on bad parameters.
 */
gpio_status_t gpio_setAlternateFunction(GPIO_TypeDef *port, gpio_pin_t pin, uint8_t alt_func);


#endif /* GPIO_DRIVER_H */