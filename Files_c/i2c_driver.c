#include "i2c_driver.h"

#define APB1_CLK_HZ 16000000U

i2c_status_t i2c_init(const i2c_config_t *config){

    /* Rutina de verificación de configuración */
    if (config == NULL) {
        return I2C_ERR_INVALID_ARG; // Puntero nulo
    }

    if (config -> instance != I2C1 && 
        config -> instance != I2C2 && 
        config -> instance != I2C3) 
        return I2C_ERR_INVALID_ARG; // Instancia no válida
    
    if (config->clk_speed == 0 ||
        config->clk_speed > I2C_SPEED_FM_HZ)
    return I2C_ERR_INVALID_ARG;
   
    if (config -> addr_mode != I2C_ADDR_7BIT &&
        config -> addr_mode != I2C_ADDR_10BIT)
        return I2C_ERR_INVALID_ARG; // Modo de dirección no válido

    /* Habilitar el reloj en RCC para el periférico I2C seleccionado */
    if (config -> instance == I2C1) {
        RCC_PERIPH() -> APB1ENR |= RCC_APB1ENR_I2C1EN;
    } else if (config -> instance == I2C2) {
        RCC_PERIPH() -> APB1ENR |= RCC_APB1ENR_I2C2EN;
    } else if (config -> instance == I2C3) {
        RCC_PERIPH() -> APB1ENR |= RCC_APB1ENR_I2C3EN;
    }

    /* Reinicio del periférico I2C */
    I2C_RegDef_t *i2c = config -> instance;

    i2c -> CR1 |= I2C_CR1_SWRST; // Set SWRST bit
    i2c -> CR1 &= ~I2C_CR1_SWRST; // Clear SWRST bit

    /* Configurar el reloj y el modo de operación */

    i2c->CR1 &= ~I2C_CR1_PE; // Deshabilitar el periférico para configurar

    /* CR2 — frecuencia APB1 en MHz */
    i2c->CR2 &= ~I2C_CR2_FREQ_MSK;
    i2c->CR2 |= (APB1_CLK_HZ / 1000000UL) & I2C_CR2_FREQ_MSK;

    /* CCR — divisor de clock */
    uint32_t ccr_val;
    if (config->clk_speed <= I2C_SPEED_SM_HZ) {
        i2c->CCR &= ~I2C_CCR_FS;                            // SM
        ccr_val = APB1_CLK_HZ / (2U * config->clk_speed);
    } else {
        i2c->CCR |= I2C_CCR_FS;                             // FM
        ccr_val = APB1_CLK_HZ / (3U * config->clk_speed);
    }
    i2c->CCR &= ~I2C_CCR_CCR_MSK;
    i2c->CCR |= (ccr_val & I2C_CCR_CCR_MSK);

    /* TRISE */
    if (config->clk_speed <= I2C_SPEED_SM_HZ)
        i2c->TRISE = (APB1_CLK_HZ / 1000000UL) + 1U;
    else
        i2c->TRISE = (APB1_CLK_HZ / 1000000UL * 300U / 1000U) + 1U;

    /* Habilitar periférico */
    i2c->CR1 |= I2C_CR1_PE;
    
    return I2C_OK;
}


i2c_status_t i2c_writeRegDevice(I2C_RegDef_t  *instance,
                                 uint16_t       dev_addr,
                                 uint8_t        reg_addr,
                                 const uint8_t *data,
                                 size_t         len)
{
    /* Verificación de argumentos */
    if (instance == NULL || data == NULL || len == 0) {
        return I2C_ERR_INVALID_ARG; // Argumentos no válidos
    }

    if (dev_addr > 0x7F) {
        return I2C_ERR_INVALID_ARG; // Dirección de dispositivo no válida
    }

    if (instance!= I2C1 && 
        instance != I2C2 && 
        instance != I2C3) {
        return I2C_ERR_INVALID_ARG; // Instancia no válida
    }

    /* Esperar a que el bus esté libre */
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (instance->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Preparación de la transacción I2C */
    instance->CR1 |= I2C_CR1_START; // Generar condición de START

    /* Esperar a que se genere el START */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envía la dirección del dispositivo */
    instance->DR = (uint8_t)(dev_addr << 1); 

    /* Esperar a que se envíe la dirección */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_ADDR)) {

        if (instance->SR1 & I2C_SR1_AF) {       // NACK — dispositivo no responde
            instance->CR1 |= I2C_CR1_STOP;
            instance->SR1 &= ~I2C_SR1_AF;
            return I2C_ERR_NACK;
        }

        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Limpiar el bit ADDR */
    (void)instance->SR1;
    (void)instance->SR2;

    /* Se envía la dirección del registro */
    instance->DR = reg_addr;

    /* Esperar a que se envíe la dirección del registro */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_TXE)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envían los datos */
    for (size_t i = 0; i < len; i++) {
        instance->DR = data[i];
        timeout = I2C_TIMEOUT_MAX;
        while (!(instance->SR1 & I2C_SR1_BTF)) {

            if (instance->SR1 & I2C_SR1_AF) {   // NACK — byte rechazado
                instance->CR1 |= I2C_CR1_STOP;
                instance->SR1 &= ~I2C_SR1_AF;
                return I2C_ERR_NACK;
            }
            if (timeout-- == 0) {
                return I2C_ERR_TIMEOUT; // Timeout
            }

        }
    }

    /* Generar condición de STOP */
    instance->CR1 |= I2C_CR1_STOP;

    return I2C_OK;
}


i2c_status_t i2c_writeDevice(I2C_RegDef_t  *instance,
                              uint16_t       dev_addr,
                              const uint8_t *data,
                              size_t         len)
{

        /* Verificación de argumentos */
    if (instance == NULL || data == NULL || len == 0) {
        return I2C_ERR_INVALID_ARG; // Argumentos no válidos
    }

    if (dev_addr > 0x7F) {
        return I2C_ERR_INVALID_ARG; // Dirección de dispositivo no válida
    }

    if (instance!= I2C1 && 
        instance != I2C2 && 
        instance != I2C3) {
        return I2C_ERR_INVALID_ARG; // Instancia no válida
    }

    /* Esperar a que el bus esté libre */
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (instance->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Preparación de la transacción I2C */
    instance->CR1 |= I2C_CR1_START; // Generar condición de START

    /* Esperar a que se genere el START */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envía la dirección del dispositivo */
    instance->DR = (uint8_t)((dev_addr << 1)| 0); 

    /* Esperar a que se envíe la dirección */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_ADDR)) {

        if (instance->SR1 & I2C_SR1_AF) {       // NACK — dispositivo no responde
            instance->CR1 |= I2C_CR1_STOP;
            instance->SR1 &= ~I2C_SR1_AF;
            return I2C_ERR_NACK;
        }

        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Limpiar el bit ADDR */
    (void)instance->SR1;
    (void)instance->SR2;
    

    /* Se envían los datos */
    for (size_t i = 0; i < len; i++) {
        instance->DR = data[i];
        timeout = I2C_TIMEOUT_MAX;

        if (i < len - 1) {
            /* Bytes intermedios: esperar BTF */
            while (!(instance->SR1 & I2C_SR1_BTF)) {
                if (instance->SR1 & I2C_SR1_AF) {
                    instance->CR1 |= I2C_CR1_STOP;
                    instance->SR1 &= ~I2C_SR1_AF;
                    return I2C_ERR_NACK;
                }
                if (timeout-- == 0) return I2C_ERR_TIMEOUT;
            }
        } else {
            /* Último byte: esperar TXE en lugar de BTF */
            while (!(instance->SR1 & I2C_SR1_TXE)) {
                if (instance->SR1 & I2C_SR1_AF) {
                    instance->CR1 |= I2C_CR1_STOP;
                    instance->SR1 &= ~I2C_SR1_AF;
                    return I2C_ERR_NACK;
                }
                if (timeout-- == 0) return I2C_ERR_TIMEOUT;
            }
        }
    }
    instance->CR1 |= I2C_CR1_STOP;

    return I2C_OK;

}

i2c_status_t i2c_readRegDevice(I2C_RegDef_t *instance,
                                uint16_t      dev_addr,
                                uint8_t       reg_addr,
                                uint8_t      *buf,
                                size_t        len)
{
    /* Verificación de argumentos */
    if (instance == NULL || buf == NULL || len == 0) {
        return I2C_ERR_INVALID_ARG; // Argumentos no válidos
    }

    if (dev_addr > 0x7F) {
        return I2C_ERR_INVALID_ARG; // Dirección de dispositivo no válida
    }

    if (instance!= I2C1 && 
        instance != I2C2 && 
        instance != I2C3) {
        return I2C_ERR_INVALID_ARG; // Instancia no válida
    }

    /* Esperar a que el bus esté libre */
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (instance->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Preparación de la transacción I2C */
    instance->CR1 |= I2C_CR1_START; // Generar condición de START

    /* Esperar a que se genere el START */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envía la dirección del dispositivo */
    instance->DR = (uint8_t)(dev_addr << 1); 

    /* Esperar a que se envíe la dirección */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_ADDR)) {

        if (instance->SR1 & I2C_SR1_AF) {       // NACK — dispositivo no responde
            instance->CR1 |= I2C_CR1_STOP;
            instance->SR1 &= ~I2C_SR1_AF;
            return I2C_ERR_NACK;
        }

        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Limpiar el bit ADDR */
    (void)instance->SR1;
    (void)instance->SR2;

    /* Se envía la dirección del registro */
    instance->DR = reg_addr;

    /* Esperar a que se envíe la dirección del registro */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_TXE)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Repeated Start */
    instance->CR1 |= I2C_CR1_START; // Generar condición de START

    
    /* Esperar a que se genere el START */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envía la dirección en modo lectura */
    instance->DR = (uint8_t)((dev_addr << 1) | 0x01); // LSB en 1 para lectura
    
    /* Esperar a que se envíe la dirección */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_ADDR)) {

        if (instance->SR1 & I2C_SR1_AF) {       // NACK — dispositivo no responde
            instance->CR1 |= I2C_CR1_STOP;
            instance->SR1 &= ~I2C_SR1_AF;
            return I2C_ERR_NACK;
        }

        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Limpiar el bit ADDR */
    (void)instance->SR1;
    (void)instance->SR2;

    /* Se habilita señal ACK para el dispositivo */
    instance->CR1 |= I2C_CR1_ACK;

        /* Lectura de los bytes recibidos */
    for (size_t i = 0; i < len; i++) {

        /* Para el último byte, se deshabilita ACK y se genera STOP 
        para liberar el bus                                      */
        if (i == len - 1) {
            instance->CR1 &= ~I2C_CR1_ACK;  // NACK al dispositivo
            instance->CR1 |= I2C_CR1_STOP;  // liberar bus
        }

        /* Esperar a que el byte esté disponible en DR */
        timeout = I2C_TIMEOUT_MAX;
        while (!(instance->SR1 & I2C_SR1_RXNE)) {
            if (timeout-- == 0) return I2C_ERR_TIMEOUT;
        }

        /* Leer el byte recibido */
        buf[i] = (uint8_t)instance->DR;
    }

    return I2C_OK;
}

i2c_status_t i2c_readDevice(I2C_RegDef_t *instance,
                             uint16_t      dev_addr,
                             uint8_t      *buf,
                             size_t        len)
{

    /* Verificación de argumentos */
    if (instance == NULL || buf == NULL || len == 0) {
        return I2C_ERR_INVALID_ARG; // Argumentos no válidos
    }

    if (dev_addr > 0x7F) {
        return I2C_ERR_INVALID_ARG; // Dirección de dispositivo no válida
    }

    if (instance!= I2C1 && 
        instance != I2C2 && 
        instance != I2C3) {
        return I2C_ERR_INVALID_ARG; // Instancia no válida
    }

    /* Esperar a que el bus esté libre */
    uint32_t timeout = I2C_TIMEOUT_MAX;
    while (instance->SR2 & I2C_SR2_BUSY) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Generar condición de START */
    instance->CR1 |= I2C_CR1_START; 

    /* Esperar a que se genere el START */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_SB)) {
        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Se envía la dirección en modo lectura */
    instance->DR = (uint8_t)((dev_addr << 1) | 0x01); // LSB en 1 para lectura
    
    /* Esperar a que se envíe la dirección */
    timeout = I2C_TIMEOUT_MAX;
    while (!(instance->SR1 & I2C_SR1_ADDR)) {

        if (instance->SR1 & I2C_SR1_AF) {       // NACK — dispositivo no responde
            instance->CR1 |= I2C_CR1_STOP;
            instance->SR1 &= ~I2C_SR1_AF;
            return I2C_ERR_NACK;
        }

        if (timeout-- == 0) {
            return I2C_ERR_TIMEOUT; // Timeout
        }
    }

    /* Limpiar el bit ADDR */
    (void)instance->SR1;
    (void)instance->SR2;

    /* Se habilita señal ACK para el dispositivo */
    instance->CR1 |= I2C_CR1_ACK;

        /* Lectura de los bytes recibidos */
    for (size_t i = 0; i < len; i++) {

        /* Para el último byte, se deshabilita ACK y se genera STOP 
        para liberar el bus                                      */
        if (i == len - 1) {
            instance->CR1 &= ~I2C_CR1_ACK;  // NACK al dispositivo
            instance->CR1 |= I2C_CR1_STOP;  // liberar bus
        }

        /* Esperar a que el byte esté disponible en DR */
        timeout = I2C_TIMEOUT_MAX;
        while (!(instance->SR1 & I2C_SR1_RXNE)) {
            if (timeout-- == 0) return I2C_ERR_TIMEOUT;
        }

        /* Leer el byte recibido */
        buf[i] = (uint8_t)instance->DR;
    }

    return I2C_OK;
}


