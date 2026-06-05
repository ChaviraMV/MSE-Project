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
* @file    sensor.h
* @brief   Analog Sensor Module - Header
*
* High-level abstraction for reading an analog sensor (Grove GSR)
* using the ADC driver. Internally uses ADC1 Channel 0 on pin PA0.
*
* Pin mapping (Nucleo-F411RE):
*   PA0 -> ADC1_IN0 -> Grove GSR SIG output (power with 3.3V)
*
* @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
* @date    04/29/2025
*/
#ifndef SENSOR_H
#define SENSOR_H

#include "adc_driver.h"

/* -----------------------------------------------------------------------
 * Hardware Mapping - GSR
 * --------------------------------------------------------------------- */
#define SENSOR_ADC_INSTANCE   ADC_INSTANCE_1
#define SENSOR_ADC_CHANNEL    0U
#define SENSOR_SAMPLE_TIME    ADC_SAMPLETIME_480CYCLES
#define SENSOR_ADC_MAX_VALUE  4095U

/* -----------------------------------------------------------------------
 * Hardware Mapping - Heart Rate (PA1)
 * --------------------------------------------------------------------- */
#define HR_ADC_INSTANCE    ADC_INSTANCE_1
#define HR_ADC_CHANNEL     1U
#define HR_SAMPLE_TIME     ADC_SAMPLETIME_480CYCLES
#define HR_THRESHOLD       3000U

/* -----------------------------------------------------------------------
 * Type Definitions
 * --------------------------------------------------------------------- */
typedef enum {
    SENSOR_OK      = 0,
    SENSOR_ERROR   = 1,
    SENSOR_INVALID = 2
} Sensor_Status_t;

typedef struct {
    ADC_Instance_t   adcInstance;
    uint8_t          adcChannel;
    ADC_SampleTime_t sampleTime;
} Sensor_Config_t;

/* -----------------------------------------------------------------------
 * Function Prototypes
 * --------------------------------------------------------------------- */
Sensor_Status_t sensor_init(const Sensor_Config_t *config);
Sensor_Status_t sensor_startConversion(void);
Sensor_Status_t sensor_readValue(uint16_t *value);

#endif /* SENSOR_H */