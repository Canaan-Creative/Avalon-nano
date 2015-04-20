#
# Author: Mikeqin <Fengling.Qin@gmail.com>
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

SW_NAME = avalon_nano

# RELEASE / DEBUG
SW_VERSION = RELEASE

DATE:=$(shell date +%Y-%m-%d)
OS:=$(shell uname -s)
GIT_VERSION:=$(shell git rev-parse HEAD | cut -c 1-7)
GIT_STATUS:=$(shell ([ -z "`git status -s -uno`" ] && echo 0) || echo +)

include make/platform_lpc11u14.mk

INCLUDES = -I./code/inc -I./nxplib/lpc_chip_11uxx_lib/inc -I./nxplib/nxp_lpcxpresso_11u14_board_lib/inc -I./nxplib/lpc_chip_11uxx_lib/inc/usbd
SRCS = $(wildcard ./code/src/*.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
LDCFLAGS += -nostdlib -Xlinker -Map=$(SW_NAME).map -Xlinker --gc-sections -mcpu=cortex-m0 -mthumb -T $(SW_NAME).ld
ifeq "$(SW_VERSION)" "DEBUG"
LDCFLAGS += -lcr_semihost -lcr_c -lcr_eabihelpers
else
LDCFLAGS += -lcr_nohost -lcr_c -lcr_eabihelpers
endif
LDLIBS += -lnxp_lpcxpresso_11u14_board_lib -llpc_chip_11uxx_lib -lusbd_11uxx_lib
LIBPATH += -L./nxplib/lpc_chip_11uxx_lib/libs -L./nxplib/nxp_lpcxpresso_11u14_board_lib/libs
CFLAGS += -DAVAU_VERSION='"3U1504-$(GIT_VERSION)$(GIT_STATUS)"'

all: $(SW_NAME).axf
	mkdir -p bin/$(DATE)
	cp $^ bin/$(DATE)
ifeq "$(OS)" "Darwin"
	cd bin/$(DATE) && md5 $^ > md5sums
else
        # linux
	cd bin/$(DATE) && md5sum -b $^ > md5sums
endif
	@echo "Ver:$(SW_VERSION) generate finsh"

$(SW_NAME).axf:	$(OBJS) lpc_chip_11uxx_lib nxp_lpcxpresso_11u14_board_lib
	$(CC) -Wl,--start-group $(LDCFLAGS) $(LIBPATH) $(OBJS) $(LDLIBS) -Wl,--end-group -o $(SW_NAME).axf

lpc_chip_11uxx_lib:
	make -C nxplib/lpc_chip_11uxx_lib

nxp_lpcxpresso_11u14_board_lib:
	make -C nxplib/nxp_lpcxpresso_11u14_board_lib

%.o : %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(PLATFORM_CFLAGS) $(INCLUDES)

reflash: $(SW_NAME).axf
	@crt_emu_cm_redlink -flash-load-exec $(SW_NAME).axf -g -2  -vendor=NXP -pLPC11U14/201 -flash-driver=LPC11_12_13_32K_4K.cfx

rungdb:
	@echo "Find the keypoint at https://en.bitcoin.it/wiki/Avalon_nano#How_to_debug_issues_with_gdb_.28Works_on_OSX.29"
clean:
	@make -C nxplib/lpc_chip_11uxx_lib clean
	@make -C nxplib/nxp_lpcxpresso_11u14_board_lib clean
	-@rm -f $(OBJS) $(SW_NAME).axf $(SW_NAME).map
	@echo "Clean finish"
