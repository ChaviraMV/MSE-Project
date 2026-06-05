# Makefile — LAB4 GSR Detector de Mentiras (STM32F411RE Nucleo)
# Estructura: Files_c/ | Files_h/ | STM_files/ | CMSIS/

include sources.mk

EXEC        = app.elf
LINKER_FILE = $(STM_DIR)/stm32f4.ld
CPU         = cortex-m4
ARCH        = armv7e-m
SPECS       = nosys.specs
FPU         = fpv4-sp-d16

ARCHFLAGS = -mcpu=$(CPU) -mthumb -march=$(ARCH) \
            -mfloat-abi=hard -mfpu=$(FPU) --specs=$(SPECS)

OBJS := $(SRCS:.c=.o)

CC = arm-none-eabi-gcc

CFLAGS  = -g -O0 -std=c99 -Werror -Wall -DSTM32F411xE $(ARCHFLAGS)
LDFLAGS = -nostdlib -T $(LINKER_FILE)

%.o : %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

.PHONY: build
build : $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@

.PHONY: flash
flash :
	openocd -f board/st_nucleo_f4.cfg \
	  -c "reset_config srst_only connect_assert_srst" \
	  -c "program $(EXEC) verify reset" \
	  -c shutdown

.PHONY: clean
clean :
	rm -f $(OBJS) $(EXEC)
