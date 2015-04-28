#
# Author: Mikeqin <Fengling.Qin@gmail.com>
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
SIZE := $(CROSS_COMPILE)size
AR := $(CROSS_COMPILE)ar

COMMON_CFLAGS = -D__REDLIB__ -D__CODE_RED -DCORE_M0 -D__REDLIB__ \
		-fmessage-length=0 -fno-builtin -mcpu=cortex-m0 -mthumb \
		-specs=redlib.specs

ifeq "$(SW_VERSION)" "DEBUG"
# -Os -Og will not work with spi, it has a side effect 
PLATFORM_CFLAGS = -g3 -O0 -ffunction-sections -fdata-sections -DDEBUG -DDEBUG_ENABLE -DDEBUG_SEMIHOSTING $(COMMON_CFLAGS)
else
PLATFORM_CFLAGS = -O0 $(COMMON_CFLAGS)
endif

