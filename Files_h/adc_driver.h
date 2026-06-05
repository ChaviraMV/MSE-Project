/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. Users are
* permitted to modify this and use it to learn about the field of embedded
* software. Carlos Villarreal and CETYS Universidad are not liable for any
* misuse of this material.
*
*****************************************************************************/
/**
* @file adc_driver.h
* @brief Brief description of what this module does.
*
* A complete description of what this module does.
*
* @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
* @date 04/29/2025
*
*/

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

/*** Includes ***/
#include <stdint.h>
#include <stddef.h>

#include "stm32f4xx.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Global Variables ***/

/*** Function Prototypes ***/
typedef enum {
    ADC_OK      = 0, 
    ADC_ERROR   = 1,  
    ADC_INVALID = 2    
} ADC_Status_t;

typedef enum {
    ADC_INSTANCE_1 = 0,
} ADC_Instance_t;

typedef enum {
    ADC_SAMPLETIME_3CYCLES   = 0,
    ADC_SAMPLETIME_15CYCLES  = 1,
    ADC_SAMPLETIME_28CYCLES  = 2,
    ADC_SAMPLETIME_56CYCLES  = 3,
    ADC_SAMPLETIME_84CYCLES  = 4,
    ADC_SAMPLETIME_112CYCLES = 5,
    ADC_SAMPLETIME_144CYCLES = 6,
    ADC_SAMPLETIME_480CYCLES = 7
} ADC_SampleTime_t;

ADC_Status_t adc_init(ADC_Instance_t instance);

ADC_Status_t adc_enableAdc(ADC_Instance_t instance);

ADC_Status_t adc_setChannel(ADC_Instance_t instance,
                             uint8_t channel,
                             ADC_SampleTime_t sampleTime);

ADC_Status_t adc_setInjectedChannel(ADC_Instance_t instance,
                                     uint8_t channel,
                                     ADC_SampleTime_t sampleTime);

ADC_Status_t adc_startSingleConversion(ADC_Instance_t instance);

ADC_Status_t adc_startContinuousConversion(ADC_Instance_t instance);

ADC_Status_t adc_startInjectedConversion(ADC_Instance_t instance);

ADC_Status_t adc_readData(ADC_Instance_t instance,
                           uint16_t *data);

ADC_Status_t adc_readInjectedChannelData(ADC_Instance_t instance,
                                          uint8_t channel,
                                          uint16_t *data);

#endif /* ADC_DRIVER_H */
