/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
*****************************************************************************/
/**
* @file    adc_driver.c
* @brief   ADC Driver Implementation - SRS v1.0
* @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
* @date    04/29/2025
*/

/*** Includes ***/
#include "adc_driver.h"
#include "stm32f4xx.h"
#include "gpio_driver.h"

/*** Preprocessor Definitions ***/
#define ADC_MAX_CHANNEL              18U
#define ADC_MAX_INJECTED_CHANNEL      4U
#define ADC_CR2_ADON_BIT         (1U << 0)
#define ADC_CR2_CONT_BIT         (1U << 1)
#define ADC_CR2_SWSTART_BIT      (1U << 30)
#define ADC_CR2_JSWSTART_BIT     (1U << 22)
#define ADC_SR_EOC_BIT           (1U << 1)
#define ADC_SR_JEOC_BIT          (1U << 2)
#define RCC_APB2ENR_ADC1EN_BIT   (1U << 8)

/*** Type Prototypes ***/
typedef struct {
    ADC_TypeDef *reg;
    uint32_t     rccBit;
} ADC_InstanceMap_t;

/*** Local Variables ***/
static const ADC_InstanceMap_t adcInstanceMap[] = {
    { ADC1, RCC_APB2ENR_ADC1EN_BIT }
};

/*** External Variables ***/
/* None */

/*** Function Prototypes ***/
static ADC_Status_t adc_validateInstance(ADC_Instance_t instance);
static ADC_Status_t adc_validateChannel(uint8_t channel);

/*** Function Definitions ***/
static ADC_Status_t adc_validateInstance(ADC_Instance_t instance)
{
    if (instance > ADC_INSTANCE_1) { return ADC_INVALID; }
    return ADC_OK;
}

static ADC_Status_t adc_validateChannel(uint8_t channel)
{
    if (channel > ADC_MAX_CHANNEL) { return ADC_INVALID; }
    return ADC_OK;
}

ADC_Status_t adc_init(ADC_Instance_t instance)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }

    /* Habilitar reloj ADC1 */
    RCC->APB2ENR |= adcInstanceMap[instance].rccBit;

    /* Habilitar reloj GPIOA sin resetear MODER */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Configurar PA0 como analógico */
    GPIOA->MODER |= (0x3U << (0U * 2U));   /* bits 1:0 = 11 → analógico */

    /* Resetear registros ADC */
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    adc->CR1 = 0x00000000U;
    adc->CR2 = 0x00000000U;

    return ADC_OK;
}

ADC_Status_t adc_enableAdc(ADC_Instance_t instance)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    adcInstanceMap[instance].reg->CR2 |= ADC_CR2_ADON_BIT;
    return ADC_OK;
}

ADC_Status_t adc_setChannel(ADC_Instance_t instance,
                             uint8_t channel,
                             ADC_SampleTime_t sampleTime)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    if (adc_validateChannel(channel)   != ADC_OK) { return ADC_INVALID; }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    adc->SQR1 = 0x00000000U;
    adc->SQR3 = (uint32_t)(channel & 0x1FU);
    if (channel <= 9U) {
        adc->SMPR2 &= ~(0x7U << (channel * 3U));
        adc->SMPR2 |=  ((uint32_t)sampleTime << (channel * 3U));
    } else {
        uint8_t offset = channel - 10U;
        adc->SMPR1 &= ~(0x7U << (offset * 3U));
        adc->SMPR1 |=  ((uint32_t)sampleTime << (offset * 3U));
    }
    return ADC_OK;
}

ADC_Status_t adc_setInjectedChannel(ADC_Instance_t instance,
                                     uint8_t channel,
                                     ADC_SampleTime_t sampleTime)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    if (adc_validateChannel(channel)   != ADC_OK) { return ADC_INVALID; }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    adc->JSQR = 0x00000000U;
    adc->JSQR |= (uint32_t)((channel & 0x1FU) << 10U);
    if (channel <= 9U) {
        adc->SMPR2 &= ~(0x7U << (channel * 3U));
        adc->SMPR2 |=  ((uint32_t)sampleTime << (channel * 3U));
    } else {
        uint8_t offset = channel - 10U;
        adc->SMPR1 &= ~(0x7U << (offset * 3U));
        adc->SMPR1 |=  ((uint32_t)sampleTime << (offset * 3U));
    }
    return ADC_OK;
}

ADC_Status_t adc_startSingleConversion(ADC_Instance_t instance)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    adc->CR2 &= ~ADC_CR2_CONT_BIT;
    adc->CR2 |=  ADC_CR2_SWSTART_BIT;
    return ADC_OK;
}

ADC_Status_t adc_startContinuousConversion(ADC_Instance_t instance)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    adc->CR2 |= ADC_CR2_CONT_BIT;
    adc->CR2 |= ADC_CR2_SWSTART_BIT;
    return ADC_OK;
}

ADC_Status_t adc_startInjectedConversion(ADC_Instance_t instance)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    adcInstanceMap[instance].reg->CR2 |= ADC_CR2_JSWSTART_BIT;
    return ADC_OK;
}

ADC_Status_t adc_readData(ADC_Instance_t instance, uint16_t *data)
{
    if (adc_validateInstance(instance) != ADC_OK) { return ADC_INVALID; }
    if (data == NULL)                              { return ADC_ERROR;   }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    while (!(adc->SR & ADC_SR_EOC_BIT));
    *data = (uint16_t)(adc->DR & 0x0FFFU);
    return ADC_OK;
}

ADC_Status_t adc_readInjectedChannelData(ADC_Instance_t instance,
                                          uint8_t channel,
                                          uint16_t *data)
{
    if (adc_validateInstance(instance) != ADC_OK)            { return ADC_INVALID; }
    if (channel == 0U || channel > ADC_MAX_INJECTED_CHANNEL) { return ADC_INVALID; }
    if (data == NULL)                                        { return ADC_ERROR;   }
    ADC_TypeDef *adc = adcInstanceMap[instance].reg;
    while (!(adc->SR & ADC_SR_JEOC_BIT));
    switch (channel) {
        case 1U: *data = (uint16_t)(adc->JDR1 & 0x0FFFU); break;
        case 2U: *data = (uint16_t)(adc->JDR2 & 0x0FFFU); break;
        case 3U: *data = (uint16_t)(adc->JDR3 & 0x0FFFU); break;
        case 4U: *data = (uint16_t)(adc->JDR4 & 0x0FFFU); break;
        default: return ADC_ERROR;
    }
    return ADC_OK;
}