/**
 * @file    tim_driver.c
 * @brief   TIM Driver Implementation for STM32F4xx
 *
 * Implements all functions defined in tim_driver.h using direct
 * register access via CMSIS peripheral definitions.
 *
 * Clock assumption: 16 MHz HSI (default after reset, no PLL).
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 * @version 1.1
 */

#include "tim_driver.h"
#include <stddef.h>      

static int is_valid_tim(TIM_TypeDef *tim)
{
    if (tim == NULL)    return 0;
    if (tim == TIM1)    return 1;
    if (tim == TIM2)    return 1;
    if (tim == TIM3)    return 1;
    if (tim == TIM4)    return 1;
    if (tim == TIM5)    return 1;
    if (tim == TIM9)    return 1;
    if (tim == TIM10)   return 1;
    if (tim == TIM11)   return 1;
    return 0;
}

static int is_valid_channel(tim_channel_t channel)
{
    return (channel >= TIM_CHANNEL_1 && channel <= TIM_CHANNEL_4);
}


tim_status_t tim_init(void)
{
    RCC->APB2ENR &= ~(RCC_APB2ENR_TIM1EN  |
                      RCC_APB2ENR_TIM9EN  |
                      RCC_APB2ENR_TIM10EN |
                      RCC_APB2ENR_TIM11EN);

    RCC->APB1ENR &= ~(RCC_APB1ENR_TIM2EN |
                      RCC_APB1ENR_TIM3EN |
                      RCC_APB1ENR_TIM4EN |
                      RCC_APB1ENR_TIM5EN);
    return TIM_OK;
}


tim_status_t tim_initTimer(TIM_TypeDef *tim)
{
    if (!is_valid_tim(tim)) return TIM_INVALID;

    if      (tim == TIM1)  { RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;  }
    else if (tim == TIM9)  { RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;  }
    else if (tim == TIM10) { RCC->APB2ENR |= RCC_APB2ENR_TIM10EN; }
    else if (tim == TIM11) { RCC->APB2ENR |= RCC_APB2ENR_TIM11EN; }
    else if (tim == TIM2)  { RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  }
    else if (tim == TIM3)  { RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  }
    else if (tim == TIM4)  { RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  }
    else if (tim == TIM5)  { RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  }


    tim->CR1  = 0;
    tim->PSC  = 0;
    tim->ARR  = 0xFFFF;
    tim->CNT  = 0;
    tim->SR   = 0;

    return TIM_OK;
}


tim_status_t tim_setTimerMs(TIM_TypeDef *tim, uint32_t ms)
{
    if (!is_valid_tim(tim))     return TIM_INVALID;
    if (ms == 0 || ms > 65535) return TIM_INVALID;

    tim->PSC = (uint16_t)((TIM_SYSTEM_CLOCK_HZ / 1000U) - 1U);
    tim->ARR = (uint16_t)(ms - 1U);
    tim->CNT = 0;
    tim->SR  = 0;   

    return TIM_OK;
}


tim_status_t tim_setTimerFreq(TIM_TypeDef *tim, uint32_t freq)
{
    if (!is_valid_tim(tim)) return TIM_INVALID;
    if (freq == 0)          return TIM_INVALID;

    uint32_t period = TIM_SYSTEM_CLOCK_HZ / freq;


    uint32_t psc = 0;
    while ((period / (psc + 1U)) > 0xFFFFU)
    {
        psc++;
        if (psc > 0xFFFFU) return TIM_INVALID;
    }

    tim->PSC = (uint16_t)psc;
    tim->ARR = (uint16_t)((period / (psc + 1U)) - 1U);
    tim->CNT = 0;
    tim->SR  = 0;

    return TIM_OK;
}


tim_status_t tim_enableTimer(TIM_TypeDef *tim)
{
    if (!is_valid_tim(tim)) return TIM_INVALID;

    tim->CR1 |= TIM_CR1_CEN;

    return TIM_OK;
}


tim_status_t tim_disableTimer(TIM_TypeDef *tim)
{
    if (!is_valid_tim(tim)) return TIM_INVALID;

    tim->CR1 &= ~TIM_CR1_CEN;

    return TIM_OK;
}


tim_status_t tim_waitTimer(TIM_TypeDef *tim)
{
    if (!is_valid_tim(tim)) return TIM_INVALID;

    while (!(tim->SR & TIM_SR_UIF));    
    tim->SR &= ~TIM_SR_UIF;            

    return TIM_OK;
}


tim_status_t tim_setTimerCompareChannelValue(TIM_TypeDef *tim,
                                             tim_channel_t channel,
                                             uint32_t value)
{
    if (!is_valid_tim(tim))      return TIM_INVALID;
    if (!is_valid_channel(channel)) return TIM_INVALID;

    switch (channel)
    {
        case TIM_CHANNEL_1: tim->CCR1 = value; break;
        case TIM_CHANNEL_2: tim->CCR2 = value; break;
        case TIM_CHANNEL_3: tim->CCR3 = value; break;
        case TIM_CHANNEL_4: tim->CCR4 = value; break;
        default: return TIM_INVALID;
    }

    return TIM_OK;
}


tim_status_t tim_setTimerCompareMode(TIM_TypeDef *tim,
                                     tim_channel_t channel,
                                     tim_compare_mode_t mode)
{
    if (!is_valid_tim(tim))         return TIM_INVALID;
    if (!is_valid_channel(channel)) return TIM_INVALID;
    if (mode > TIM_COMPARE_MODE_PWM2) return TIM_INVALID;

    switch (channel)
    {
        case TIM_CHANNEL_1:
            tim->CCMR1 &= ~TIM_CCMR1_OC1M;
            tim->CCMR1 |= ((uint32_t)mode << 4U);
            tim->CCMR1 |= TIM_CCMR1_OC1PE;  /* Enable preload */
            break;
        case TIM_CHANNEL_2:
            tim->CCMR1 &= ~TIM_CCMR1_OC2M;
            tim->CCMR1 |= ((uint32_t)mode << 12U);
            tim->CCMR1 |= TIM_CCMR1_OC2PE;
            break;
        case TIM_CHANNEL_3:
            tim->CCMR2 &= ~TIM_CCMR2_OC3M;
            tim->CCMR2 |= ((uint32_t)mode << 4U);
            tim->CCMR2 |= TIM_CCMR2_OC3PE;
            break;
        case TIM_CHANNEL_4:
            tim->CCMR2 &= ~TIM_CCMR2_OC4M;
            tim->CCMR2 |= ((uint32_t)mode << 12U);
            tim->CCMR2 |= TIM_CCMR2_OC4PE;
            break;
        default: return TIM_INVALID;
    }


    tim->CR1 |= TIM_CR1_ARPE;

    return TIM_OK;
}


tim_status_t tim_enableTimerCompareChannel(TIM_TypeDef *tim,
                                           tim_channel_t channel)
{
    if (!is_valid_tim(tim))         return TIM_INVALID;
    if (!is_valid_channel(channel)) return TIM_INVALID;

    /* CCxE bits are at positions 0, 4, 8, 12 in CCER */
    tim->CCER |= (1U << ((channel - 1U) * 4U));

    return TIM_OK;
}


tim_status_t tim_disableTimerCompareChannel(TIM_TypeDef *tim,
                                            tim_channel_t channel)
{
    if (!is_valid_tim(tim))         return TIM_INVALID;
    if (!is_valid_channel(channel)) return TIM_INVALID;

    tim->CCER &= ~(1U << ((channel - 1U) * 4U));

    return TIM_OK;
}
