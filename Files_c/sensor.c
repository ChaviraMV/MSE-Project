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
* @file    sensor.c
* @brief   Analog Sensor Module Implementation
*
* Provides a high-level abstraction over the ADC driver to read
* analog sensor values from a potentiometer connected to PA0
* (ADC1 Channel 0) on the STM32F411RE Nucleo board.
*
* @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
* @date    04/29/2025
*/

/*** Includes ***/
#include "sensor.h"

/*** Preprocessor Definitions ***/
/* None */

/*** Type Prototypes ***/
/* None */

/*** Local Variables ***/
static Sensor_Config_t s_sensorConfig;   /* configuración activa del sensor */

/*** External Variables ***/
/* None */

/*** Function Prototypes ***/
/* None */

/*** Function Definitions ***/

Sensor_Status_t sensor_init(const Sensor_Config_t *config)
{
    /* Validar puntero — NFR-2 */
    if (config == NULL)
    {
        return SENSOR_INVALID;
    }

    /* Guardar configuración localmente */
    s_sensorConfig = *config;

    /* FR-1: Inicializar ADC (reloj + reset) */
    if (adc_init(config->adcInstance) != ADC_OK)
    {
        return SENSOR_ERROR;
    }

    /* FR-2: Encender ADC (ADON = 1) */
    if (adc_enableAdc(config->adcInstance) != ADC_OK)
    {
        return SENSOR_ERROR;
    }

    /* FR-3: Configurar canal regular (SQR3 + SMPR) */
    if (adc_setChannel(config->adcInstance,
                       config->adcChannel,
                       config->sampleTime) != ADC_OK)
    {
        return SENSOR_ERROR;
    }

    return SENSOR_OK;
}

Sensor_Status_t sensor_startConversion(void)
{
    /* FR-5: Disparar UNA conversión (SWSTART) */
    if (adc_startSingleConversion(s_sensorConfig.adcInstance) != ADC_OK)
    {
        return SENSOR_ERROR;
    }

    return SENSOR_OK;
}

Sensor_Status_t sensor_readValue(uint16_t *value)
{
    /* Validar puntero */
    if (value == NULL)
    {
        return SENSOR_INVALID;
    }

    /* FR-8: Esperar EOC y leer resultado (0–4095) */
    if (adc_readData(s_sensorConfig.adcInstance, value) != ADC_OK)
    {
        return SENSOR_ERROR;
    }

    return SENSOR_OK;
}