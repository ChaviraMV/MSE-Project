# sources.mk

SRC_DIR     = ./Files_c
STM_DIR     = ./STM_files
INCLUDE_DIR = ./Files_h
CMSIS_DIR   = ./CMSIS

SRCS = \
$(SRC_DIR)/main.c           \
$(SRC_DIR)/gpio_driver.c    \
$(SRC_DIR)/adc_driver.c     \
$(SRC_DIR)/tim_driver.c     \
$(SRC_DIR)/i2c_driver.c     \
$(SRC_DIR)/lcd_ssd1306.c    \
$(SRC_DIR)/uart.c           \
$(SRC_DIR)/serial.c         \
$(SRC_DIR)/sensor.c         \
$(SRC_DIR)/utils.c          \
$(STM_DIR)/stm32.startup.c  \
$(STM_DIR)/system_stm32f4xx.c

INCLUDES = \
-I$(INCLUDE_DIR)            \
-I$(CMSIS_DIR)              \
-I$(CMSIS_DIR)/STM32F4xx
