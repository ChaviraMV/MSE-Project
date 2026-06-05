/**
 * @file    gpio_driver.c
 * @brief   GPIO Driver usando CMSIS para STM32F4
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @version 1.0
 */
#include "gpio_driver.h"

/* -----------------------------------------------------------------------
 * Private Helper
 * --------------------------------------------------------------------- */


static int is_valid(GPIO_TypeDef *port, gpio_pin_t pin)
{
    // CMSIS 
    if (port == NULL || pin > 15) return 0;
    return 1;
}

/* -----------------------------------------------------------------------
 * Public Functions
 * --------------------------------------------------------------------- */


gpio_status_t gpio_init(void)
{
    /* Clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN;

    /* Reset*/
    GPIOA->MODER = 0xFFFFFFFFU;
    GPIOB->MODER = 0xFFFFFFFFU;
    GPIOC->MODER = 0xFFFFFFFFU;
    GPIOD->MODER = 0xFFFFFFFFU;
    GPIOE->MODER = 0xFFFFFFFFU;

    return GPIO_OK;
}
gpio_status_t gpio_initPort(GPIO_TypeDef *port)
{
    if (port == NULL) return GPIO_INVALID;


    if      (port == GPIOA) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    else if (port == GPIOB) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    else if (port == GPIOC) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    else if (port == GPIOD) RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    else if (port == GPIOE) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    else return GPIO_INVALID;

    return GPIO_OK;
}

gpio_status_t gpio_setPinMode(GPIO_TypeDef *port, gpio_pin_t pin, gpio_mode_t mode)
{
    if (!is_valid(port, pin)) return GPIO_INVALID;
    port->MODER &= ~(0x3U << (pin * 2U));
    port->MODER |=  ((uint32_t)mode << (pin * 2U));

    return GPIO_OK;
}

gpio_status_t gpio_setPin(GPIO_TypeDef *port, gpio_pin_t pin)
{
    if (!is_valid(port, pin)) return GPIO_INVALID;
    port->BSRR = (1U << pin);
    return GPIO_OK;
}


gpio_status_t gpio_clearPin(GPIO_TypeDef *port, gpio_pin_t pin)
{
    if (!is_valid(port, pin)) return GPIO_INVALID;
    port->BSRR = (1U << (pin + 16U));
    return GPIO_OK;
}


gpio_status_t gpio_togglePin(GPIO_TypeDef *port, gpio_pin_t pin)
{
    if (!is_valid(port, pin)) return GPIO_INVALID;
    port->ODR ^= (1U << pin);
    return GPIO_OK;
}

gpio_status_t gpio_readPin(GPIO_TypeDef *port, gpio_pin_t pin, gpio_pin_state_t *state)
{
    if (!is_valid(port, pin) || state == NULL) return GPIO_INVALID;
    if (port->IDR & (1U << pin)) {
        *state = GPIO_PIN_HIGH;
    } else {
        *state = GPIO_PIN_LOW;
    }

    return GPIO_OK;
}
gpio_status_t gpio_setAlternateFunction(GPIO_TypeDef *port, gpio_pin_t pin, uint8_t alt_func)
{
    if (!is_valid(port, pin)) return GPIO_INVALID;
    if (alt_func > 15U)       return GPIO_INVALID;

    port->MODER &= ~(0x3U << (pin * 2U));
    port->MODER |=  (0x2U << (pin * 2U));


    if (pin < 8U)
    {
        port->AFR[0] &= ~(0xFU << (pin * 4U));
        port->AFR[0] |=  ((uint32_t)alt_func << (pin * 4U));
    }
    else
    {
        uint8_t pin_offset = pin - 8U;
        port->AFR[1] &= ~(0xFU << (pin_offset * 4U));
        port->AFR[1] |=  ((uint32_t)alt_func << (pin_offset * 4U));
    }

    return GPIO_OK;
}