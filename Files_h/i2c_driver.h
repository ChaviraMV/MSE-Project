/**
 * @file    i2c_driver.h
 * @brief   Self-contained I2C Driver for STM32F411xC/E
 *
 * Defines peripheral structs, register bit masks, base addresses,
 * and the public driver API.  The only external dependency is the
 * C standard library (<stdint.h>, <stddef.h>).
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 *          Leonardo Ivan García Espinosa 
 *          Mario Morales Vega
 * @version 1.0
 *
 * @ref     IEEE Std 830-1998
 * @ref     STM32F411RE Datasheet
 * @ref     RM0383 STM32F411xC/E Reference Manual
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Compiler helpers
 * ============================================================ */

/** Map a peripheral base address to a typed pointer */
#define I2C_PERIPH(base)    ((I2C_RegDef_t *)(base))
#define RCC_PERIPH()        ((RCC_RegDef_t *)(RCC_BASE_ADDR))

/* ============================================================
 * Base addresses  (RM0383 §2.3 Memory map)
 * ============================================================ */

#define I2C1_BASE_ADDR      0x40005400UL
#define I2C2_BASE_ADDR      0x40005800UL
#define I2C3_BASE_ADDR      0x40005C00UL

#define RCC_BASE_ADDR       0x40023800UL

/* ============================================================
 * Peripheral register structures
 * ============================================================ */

/**
 * @brief  I2C peripheral register layout  (RM0383 §18.6)
 *
 * Each field maps directly to the hardware register at the
 * corresponding word offset from the peripheral base address.
 */
typedef struct
{
    volatile uint32_t CR1;    /**< 0x00 — Control register 1          */
    volatile uint32_t CR2;    /**< 0x04 — Control register 2          */
    volatile uint32_t OAR1;   /**< 0x08 — Own address register 1      */
    volatile uint32_t OAR2;   /**< 0x0C — Own address register 2      */
    volatile uint32_t DR;     /**< 0x10 — Data register               */
    volatile uint32_t SR1;    /**< 0x14 — Status register 1           */
    volatile uint32_t SR2;    /**< 0x18 — Status register 2           */
    volatile uint32_t CCR;    /**< 0x1C — Clock control register      */
    volatile uint32_t TRISE;  /**< 0x20 — TRISE register              */
    volatile uint32_t FLTR;   /**< 0x24 — FLTR register               */
} I2C_RegDef_t;

/**
 * @brief  RCC peripheral register layout — APB1ENR only  (RM0383 §6.3)
 *
 * Only the registers needed to gate the I2C clocks are defined.
 * The reserved fields preserve the correct word offsets.
 */
typedef struct
{
    volatile uint32_t CR;           /**< 0x00 */
    volatile uint32_t PLLCFGR;     /**< 0x04 */
    volatile uint32_t CFGR;        /**< 0x08 */
    volatile uint32_t CIR;         /**< 0x0C */
    volatile uint32_t AHB1RSTR;    /**< 0x10 */
    volatile uint32_t AHB2RSTR;    /**< 0x14 */
    uint32_t          _res0[2];    /**< 0x18–0x1C — reserved */
    volatile uint32_t APB1RSTR;    /**< 0x20 */
    volatile uint32_t APB2RSTR;    /**< 0x24 */
    uint32_t          _res1[2];    /**< 0x28–0x2C — reserved */
    volatile uint32_t AHB1ENR;     /**< 0x30 */
    volatile uint32_t AHB2ENR;     /**< 0x34 */
    uint32_t          _res2[2];    /**< 0x38–0x3C — reserved */
    volatile uint32_t APB1ENR;     /**< 0x40 — I2C clock enable bits  */
    volatile uint32_t APB2ENR;     /**< 0x44 */
} RCC_RegDef_t;

/* ============================================================
 * Peripheral instance shortcuts
 * ============================================================ */

#ifndef I2C1
#define I2C1    I2C_PERIPH(I2C1_BASE_ADDR)
#endif
#ifndef I2C2
#define I2C2    I2C_PERIPH(I2C2_BASE_ADDR)
#endif
#ifndef I2C3
#define I2C3    I2C_PERIPH(I2C3_BASE_ADDR)
#endif

/* ============================================================
 * CR1 — Control register 1
 * ============================================================ */
#define I2C_CR1_PE_POS          0U
#define I2C_CR1_SMBUS_POS       1U
#define I2C_CR1_SMBTYPE_POS     3U
#define I2C_CR1_ENARP_POS       4U
#define I2C_CR1_ENPEC_POS       5U
#define I2C_CR1_ENGC_POS        6U
#define I2C_CR1_NOSTRETCH_POS   7U
#define I2C_CR1_START_POS       8U
#define I2C_CR1_STOP_POS        9U
#define I2C_CR1_ACK_POS         10U
#define I2C_CR1_POS_POS         11U
#define I2C_CR1_PEC_POS         12U
#define I2C_CR1_ALERT_POS       13U
#define I2C_CR1_SWRST_POS       15U

#ifndef I2C_CR1_PE
#define I2C_CR1_PE              (1U << I2C_CR1_PE_POS)
#endif
#ifndef I2C_CR1_SMBUS
#define I2C_CR1_SMBUS           (1U << I2C_CR1_SMBUS_POS)
#endif
#ifndef I2C_CR1_SMBTYPE
#define I2C_CR1_SMBTYPE         (1U << I2C_CR1_SMBTYPE_POS)
#endif
#ifndef I2C_CR1_ENARP
#define I2C_CR1_ENARP           (1U << I2C_CR1_ENARP_POS)
#endif
#ifndef I2C_CR1_ENPEC
#define I2C_CR1_ENPEC           (1U << I2C_CR1_ENPEC_POS)
#endif
#ifndef I2C_CR1_ENGC
#define I2C_CR1_ENGC            (1U << I2C_CR1_ENGC_POS)
#endif
#ifndef I2C_CR1_NOSTRETCH
#define I2C_CR1_NOSTRETCH       (1U << I2C_CR1_NOSTRETCH_POS)
#endif
#ifndef I2C_CR1_START
#define I2C_CR1_START           (1U << I2C_CR1_START_POS)
#endif
#ifndef I2C_CR1_STOP
#define I2C_CR1_STOP            (1U << I2C_CR1_STOP_POS)
#endif
#ifndef I2C_CR1_ACK
#define I2C_CR1_ACK             (1U << I2C_CR1_ACK_POS)
#endif
#ifndef I2C_CR1_POS
#define I2C_CR1_POS             (1U << I2C_CR1_POS_POS)
#endif
#ifndef I2C_CR1_PEC
#define I2C_CR1_PEC             (1U << I2C_CR1_PEC_POS)
#endif
#ifndef I2C_CR1_ALERT
#define I2C_CR1_ALERT           (1U << I2C_CR1_ALERT_POS)
#endif
#ifndef I2C_CR1_SWRST
#define I2C_CR1_SWRST           (1U << I2C_CR1_SWRST_POS)
#endif

/* ============================================================
 * CR2 — Control register 2
 * ============================================================ */
#define I2C_CR2_FREQ_POS        0U
#define I2C_CR2_ITERREN_POS     8U
#define I2C_CR2_ITEVTEN_POS     9U
#define I2C_CR2_ITBUFEN_POS     10U
#define I2C_CR2_DMAEN_POS       11U
#define I2C_CR2_LAST_POS        12U

#define I2C_CR2_FREQ_MSK        (0x3FU << I2C_CR2_FREQ_POS)
#ifndef I2C_CR2_ITERREN
#define I2C_CR2_ITERREN         (1U << I2C_CR2_ITERREN_POS)
#endif
#ifndef I2C_CR2_ITEVTEN
#define I2C_CR2_ITEVTEN         (1U << I2C_CR2_ITEVTEN_POS)
#endif
#ifndef I2C_CR2_ITBUFEN
#define I2C_CR2_ITBUFEN         (1U << I2C_CR2_ITBUFEN_POS)
#endif
#ifndef I2C_CR2_DMAEN
#define I2C_CR2_DMAEN           (1U << I2C_CR2_DMAEN_POS)
#endif
#ifndef I2C_CR2_LAST
#define I2C_CR2_LAST            (1U << I2C_CR2_LAST_POS)
#endif

/* ============================================================
 * SR1 — Status register 1
 * ============================================================ */
#define I2C_SR1_SB_POS          0U
#define I2C_SR1_ADDR_POS        1U
#define I2C_SR1_BTF_POS         2U
#define I2C_SR1_ADD10_POS       3U
#define I2C_SR1_STOPF_POS       4U
#define I2C_SR1_RXNE_POS        6U
#define I2C_SR1_TXE_POS         7U
#define I2C_SR1_BERR_POS        8U
#define I2C_SR1_ARLO_POS        9U
#define I2C_SR1_AF_POS          10U
#define I2C_SR1_OVR_POS         11U
#define I2C_SR1_PECERR_POS      12U
#define I2C_SR1_TIMEOUT_POS     14U
#define I2C_SR1_SMBALERT_POS    15U

#ifndef I2C_SR1_SB
#define I2C_SR1_SB              (1U << I2C_SR1_SB_POS)
#endif
#ifndef I2C_SR1_ADDR
#define I2C_SR1_ADDR            (1U << I2C_SR1_ADDR_POS)
#endif
#ifndef I2C_SR1_BTF
#define I2C_SR1_BTF             (1U << I2C_SR1_BTF_POS)
#endif
#ifndef I2C_SR1_ADD10
#define I2C_SR1_ADD10           (1U << I2C_SR1_ADD10_POS)
#endif
#ifndef I2C_SR1_STOPF
#define I2C_SR1_STOPF           (1U << I2C_SR1_STOPF_POS)
#endif
#ifndef I2C_SR1_RXNE
#define I2C_SR1_RXNE            (1U << I2C_SR1_RXNE_POS)
#endif
#ifndef I2C_SR1_TXE
#define I2C_SR1_TXE             (1U << I2C_SR1_TXE_POS)
#endif
#ifndef I2C_SR1_BERR
#define I2C_SR1_BERR            (1U << I2C_SR1_BERR_POS)
#endif
#ifndef I2C_SR1_ARLO
#define I2C_SR1_ARLO            (1U << I2C_SR1_ARLO_POS)
#endif
#ifndef I2C_SR1_AF
#define I2C_SR1_AF              (1U << I2C_SR1_AF_POS)
#endif
#ifndef I2C_SR1_OVR
#define I2C_SR1_OVR             (1U << I2C_SR1_OVR_POS)
#endif
#ifndef I2C_SR1_PECERR
#define I2C_SR1_PECERR          (1U << I2C_SR1_PECERR_POS)
#endif
#ifndef I2C_SR1_TIMEOUT
#define I2C_SR1_TIMEOUT         (1U << I2C_SR1_TIMEOUT_POS)
#endif
#ifndef I2C_SR1_SMBALERT
#define I2C_SR1_SMBALERT        (1U << I2C_SR1_SMBALERT_POS)
#endif

#define I2C_SR1_ERR_MSK         (I2C_SR1_BERR   | \
                                 I2C_SR1_ARLO   | \
                                 I2C_SR1_AF     | \
                                 I2C_SR1_OVR    | \
                                 I2C_SR1_TIMEOUT)

/* ============================================================
 * SR2 — Status register 2
 * ============================================================ */
#define I2C_SR2_MSL_POS         0U
#define I2C_SR2_BUSY_POS        1U
#define I2C_SR2_TRA_POS         2U
#define I2C_SR2_GENCALL_POS     4U
#define I2C_SR2_DUALF_POS       7U

#ifndef I2C_SR2_MSL
#define I2C_SR2_MSL             (1U << I2C_SR2_MSL_POS)
#endif
#ifndef I2C_SR2_BUSY
#define I2C_SR2_BUSY            (1U << I2C_SR2_BUSY_POS)
#endif
#ifndef I2C_SR2_TRA
#define I2C_SR2_TRA             (1U << I2C_SR2_TRA_POS)
#endif
#ifndef I2C_SR2_GENCALL
#define I2C_SR2_GENCALL         (1U << I2C_SR2_GENCALL_POS)
#endif
#ifndef I2C_SR2_DUALF
#define I2C_SR2_DUALF           (1U << I2C_SR2_DUALF_POS)
#endif

/* ============================================================
 * CCR — Clock control register
 * ============================================================ */
#define I2C_CCR_CCR_POS         0U
#define I2C_CCR_DUTY_POS        14U
#define I2C_CCR_FS_POS          15U

#define I2C_CCR_CCR_MSK         (0xFFFU << I2C_CCR_CCR_POS)
#ifndef I2C_CCR_DUTY
#define I2C_CCR_DUTY            (1U << I2C_CCR_DUTY_POS)
#endif
#ifndef I2C_CCR_FS
#define I2C_CCR_FS              (1U << I2C_CCR_FS_POS)
#endif

/* ============================================================
 * OAR1 — Own address register 1
 * ============================================================ */
#define I2C_OAR1_ADD0_POS       0U
#define I2C_OAR1_ADD_7BIT_POS   1U
#define I2C_OAR1_ADDMODE_POS    15U

#ifndef I2C_OAR1_ADD0
#define I2C_OAR1_ADD0           (1U    << I2C_OAR1_ADD0_POS)
#endif
#define I2C_OAR1_ADD_7BIT       (0x7FU << I2C_OAR1_ADD_7BIT_POS)
#ifndef I2C_OAR1_ADDMODE
#define I2C_OAR1_ADDMODE        (1U    << I2C_OAR1_ADDMODE_POS)
#endif

/* ============================================================
 * RCC APB1ENR — I2C clock enable bits
 * ============================================================ */
#define RCC_APB1ENR_I2C1EN_POS  21U
#define RCC_APB1ENR_I2C2EN_POS  22U
#define RCC_APB1ENR_I2C3EN_POS  23U

#ifndef RCC_APB1ENR_I2C1EN
#define RCC_APB1ENR_I2C1EN      (1U << RCC_APB1ENR_I2C1EN_POS)
#endif
#ifndef RCC_APB1ENR_I2C2EN
#define RCC_APB1ENR_I2C2EN      (1U << RCC_APB1ENR_I2C2EN_POS)
#endif
#ifndef RCC_APB1ENR_I2C3EN
#define RCC_APB1ENR_I2C3EN      (1U << RCC_APB1ENR_I2C3EN_POS)
#endif
/* ============================================================
 * Clock speed presets
 * ============================================================ */

/** APB1 bus frequency assumed during CCR / TRISE computation (Hz) */
#define I2C_APB1_CLK_HZ     42000000U

#define I2C_SPEED_SM_HZ     100000U   /**< Standard mode — 100 kHz */
#define I2C_SPEED_FM_HZ     400000U   /**< Fast mode    — 400 kHz  */

/* ============================================================
 * Addressing mode selectors
 * ============================================================ */

#define I2C_ADDR_7BIT       0U   /**< 7-bit device addressing  */
#define I2C_ADDR_10BIT      1U   /**< 10-bit device addressing */

/* ============================================================
 * Timeout  (NFR-3: real-time constraints)
 * ============================================================ */

/** Polling iteration limit before a transfer is declared timed-out */
#define I2C_TIMEOUT_MAX     1000000U

/* ============================================================
 * Return codes  (NFR-2: error handling)
 * ============================================================ */

/**
 * @brief  Driver status codes returned by every public function.
 */
typedef enum
{
    I2C_OK               =  0,   /**< Operation succeeded               */
    I2C_ERR_INVALID_ARG  = -1,   /**< NULL pointer or illegal parameter */
    I2C_ERR_BUS_BUSY     = -2,   /**< Bus not idle at START             */
    I2C_ERR_NACK         = -3,   /**< Device sent NACK                  */
    I2C_ERR_TIMEOUT      = -4,   /**< Flag wait exceeded timeout limit  */
    I2C_ERR_BUS_ERROR    = -5,   /**< Misplaced START/STOP on the bus   */
} i2c_status_t;

/* ============================================================
 * Driver configuration structure
 * ============================================================ */

/**
 * @brief  Run-time configuration for i2c_init().
 */
typedef struct
{
    I2C_RegDef_t *instance;    /**< Peripheral pointer: I2C1, I2C2, I2C3 */
    uint32_t      clk_speed;   /**< Bus speed in Hz: I2C_SPEED_SM/FM_HZ  */
    uint8_t       addr_mode;   /**< I2C_ADDR_7BIT or I2C_ADDR_10BIT      */
} i2c_config_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * @brief  Initialize the I2C peripheral.  (FR-1, FR-2)
 *
 * 1. Enables the APB1 clock for the selected instance via RCC.
 * 2. Resets CR1 (SWRST) to clear any previous fault state.
 * 3. Programs CR2 with the APB1 frequency in MHz.
 * 4. Computes and writes CCR for the requested bus speed.
 * 5. Computes and writes TRISE.
 * 6. Sets PE to leave the peripheral ready.
 *
 * @param[in]  config  Pointer to a populated i2c_config_t.
 *                     Must not be NULL; instance must be I2C1/2/3;
 *                     clk_speed must be non-zero.
 *
 * @retval  I2C_OK              Peripheral is ready.
 * @retval  I2C_ERR_INVALID_ARG config or instance is NULL,
 *                              or clk_speed is zero.
 */
i2c_status_t i2c_init(const i2c_config_t *config);

/**
 * @brief  Write bytes to a register of an I2C device.  (FR-3, FR-4)
 *
 * Bus sequence:
 *   START → addr(W) → reg_addr → data[0] … data[len-1] → STOP
 *
 * @param[in]  instance   Peripheral pointer initialised by i2c_init().
 * @param[in]  dev_addr   Device address, unshifted (7-bit: 0x00–0x7F).
 * @param[in]  reg_addr   Target register inside the device.
 * @param[in]  data       Bytes to transmit. Must not be NULL.
 * @param[in]  len        Number of bytes (>= 1).
 *
 * @retval  I2C_OK              All bytes written and acknowledged.
 * @retval  I2C_ERR_INVALID_ARG instance or data is NULL, len is 0.
 * @retval  I2C_ERR_BUS_BUSY    SR2.BUSY set before START.
 * @retval  I2C_ERR_NACK        SR1.AF set — device did not acknowledge.
 * @retval  I2C_ERR_TIMEOUT     A flag wait exceeded I2C_TIMEOUT_MAX.
 * @retval  I2C_ERR_BUS_ERROR   SR1.BERR set during transfer.
 */
i2c_status_t i2c_writeRegDevice(I2C_RegDef_t  *instance,
                                 uint16_t       dev_addr,
                                 uint8_t        reg_addr,
                                 const uint8_t *data,
                                 size_t         len);

/**
 * @brief  Write bytes directly to an I2C device.  (FR-5, FR-6)
 *
 * Bus sequence:
 *   START → addr(W) → data[0] … data[len-1] → STOP
 *
 * @param[in]  instance   Peripheral pointer initialised by i2c_init().
 * @param[in]  dev_addr   Device address, unshifted.
 * @param[in]  data       Bytes to transmit. Must not be NULL.
 * @param[in]  len        Number of bytes (>= 1).
 *
 * @retval  See i2c_writeRegDevice().
 */
i2c_status_t i2c_writeDevice(I2C_RegDef_t  *instance,
                              uint16_t       dev_addr,
                              const uint8_t *data,
                              size_t         len);

/**
 * @brief  Read bytes from a register of an I2C device.  (FR-7, FR-8)
 *
 * Bus sequence:
 *   START → addr(W) → reg_addr → Sr → addr(R) → data[0] … data[len-1] → STOP
 *
 * @param[in]  instance   Peripheral pointer initialised by i2c_init().
 * @param[in]  dev_addr   Device address, unshifted.
 * @param[in]  reg_addr   Source register inside the device.
 * @param[out] buf        Receive buffer. Must not be NULL.
 * @param[in]  len        Number of bytes to read (>= 1).
 *
 * @retval  I2C_OK              len bytes written into buf.
 * @retval  I2C_ERR_INVALID_ARG instance or buf is NULL, len is 0.
 * @retval  I2C_ERR_BUS_BUSY    SR2.BUSY set before START.
 * @retval  I2C_ERR_NACK        Device did not acknowledge address.
 * @retval  I2C_ERR_TIMEOUT     A flag wait exceeded I2C_TIMEOUT_MAX.
 * @retval  I2C_ERR_BUS_ERROR   SR1.BERR set during transfer.
 */
i2c_status_t i2c_readRegDevice(I2C_RegDef_t *instance,
                                uint16_t      dev_addr,
                                uint8_t       reg_addr,
                                uint8_t      *buf,
                                size_t        len);

/**
 * @brief  Read bytes directly from an I2C device.  (FR-9, FR-10)
 *
 * Bus sequence:
 *   START → addr(R) → data[0] … data[len-1] → STOP
 *
 * @param[in]  instance   Peripheral pointer initialised by i2c_init().
 * @param[in]  dev_addr   Device address, unshifted.
 * @param[out] buf        Receive buffer. Must not be NULL.
 * @param[in]  len        Number of bytes to read (>= 1).
 *
 * @retval  See i2c_readRegDevice().
 */
i2c_status_t i2c_readDevice(I2C_RegDef_t *instance,
                             uint16_t      dev_addr,
                             uint8_t      *buf,
                             size_t        len);

/* ============================================================
 * Usage examples  (SRS §6 — Documentation requirement)
 * ============================================================
 *
 * --- Initialisation ---
 *
 *   i2c_config_t cfg = {
 *       .instance  = I2C1,
 *       .clk_speed = I2C_SPEED_FM_HZ,
 *       .addr_mode = I2C_ADDR_7BIT,
 *   };
 *   i2c_init(&cfg);
 *
 *
 * --- Register write (e.g. wake-up an MPU-6050) ---
 *
 *   uint8_t payload = 0x00;
 *   i2c_writeRegDevice(I2C1, 0x68, 0x6B, &payload, 1);
 *
 *
 * --- Register read (e.g. 6-axis raw data burst) ---
 *
 *   uint8_t raw[14];
 *   i2c_readRegDevice(I2C1, 0x68, 0x3B, raw, sizeof(raw));
 *
 *
 * --- Direct write (e.g. OLED SSD1306 command) ---
 *
 *   uint8_t cmd[] = { 0x00, 0xAE };
 *   i2c_writeDevice(I2C1, 0x3C, cmd, sizeof(cmd));
 *
 *
 * --- Direct read ---
 *
 *   uint8_t data[2];
 *   i2c_readDevice(I2C1, 0x48, data, sizeof(data));
 *
 * ============================================================ */

#ifdef __cplusplus
}
#endif

#endif /* I2C_DRIVER_H */