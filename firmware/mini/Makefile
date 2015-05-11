#
# Author: Mikeqin <Fengling.Qin@gmail.com>
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

FIRMWARE = avalon4_mini
FW_RELEASE = RELEASE

DATE:=$(shell date +%Y-%m-%d)
OS:=$(shell uname -s)

SRCS	= $(wildcard ./code/src/*.c)
include make/defaultc.mk

GIT_VERSION:=$(shell git rev-parse HEAD | cut -c 1-7)
GIT_STATUS:=$(shell ([ -z "`git status -s -uno`" ] && echo 0) || echo +)

CFLAGS	+= -DAVAM_VERSION='"4M1505-$(GIT_VERSION)$(GIT_STATUS)"' \
	-I./code/inc -I./nxplib/lpc_chip_11uxx_lib/inc \
	-I./nxplib/nxp_lpcxpresso_11u14_board_lib/inc \
	-I./nxplib/lpc_chip_11uxx_lib/inc/usbd

LDCFLAGS += -nostdlib -Xlinker -Map=$(FIRMWARE).map \
	-Xlinker --gc-sections -mcpu=cortex-m0 -mthumb \
	-T $(FIRMWARE).ld -lcr_c -lcr_eabihelpers

ifeq "$(FW_RELEASE)" "DEBUG"
LDCFLAGS += -lcr_semihost
else
LDCFLAGS += -lcr_nohost
endif

LDLIBS   += -lnxp_lpcxpresso_11u14_board_lib -llpc_chip_11uxx_lib \
	-lusbd_11uxx_lib
LIBPATH  += -L./nxplib/lpc_chip_11uxx_lib/libs \
	-L./nxplib/nxp_lpcxpresso_11u14_board_lib/libs

all: $(FIRMWARE).axf
	mkdir -p bin/$(DATE)
	cp $^ bin/$(DATE)
ifeq "$(OS)" "Darwin"
	cd bin/$(DATE) && md5 $^ > md5sums
else
	cd bin/$(DATE) && md5sum -b $^ > md5sums
endif
	$(SIZE) $^

$(FIRMWARE).axf: $(OBJS) lpc_chip_11uxx_lib nxp_lpcxpresso_11u14_board_lib
	$(CC) -Wl,--start-group $(LDCFLAGS) $(LIBPATH) $(OBJS) $(LDLIBS) -Wl,--end-group -o $(FIRMWARE).axf

lpc_chip_11uxx_lib:
	make -C nxplib/lpc_chip_11uxx_lib

nxp_lpcxpresso_11u14_board_lib:
	make -C nxplib/nxp_lpcxpresso_11u14_board_lib

reflash_ulink2: $(FIRMWARE).axf
	-killall -s 9 redlinkserv
	crt_emu_cm_redlink -flash-load-exec $< -g -2 -vendor=NXP -pLPC11U14/201 -wire=winUSB -s50 -flash-driver=LPC11_12_13_32K_4K.cfx

reflash_lpclink: $(FIRMWARE).axf
	-(dfu-util -d 0x0471:0xdf55 -c 0 -t 2048 -R -D "LPCXpressoWIN.enc" && sleep 1)
	crt_emu_lpc11_13_nxp -flash-load-exec $< -g -2 -vendor=NXP -pLPC11U14/201 -wire=winUSB -s50 -flash-driver=LPC11_12_13_32K_4K.cfx

erase_lpclink:
	-(dfu-util -d 0x0471:0xdf55 -c 0 -t 2048 -R -D "LPCXpressoWIN.enc" && sleep 1)
	crt_emu_lpc11_13_nxp -flash-erase -g -2 -vendor=NXP -pLPC11U14/201 -wire=winUSB -s50 -flash-driver=LPC11_12_13_32K_4K.cfx

rungdb:
	@echo "Find the keypoint at https://en.bitcoin.it/wiki/Avalon_nano#How_to_debug_issues_with_gdb_.28Works_on_OSX.29"

clean:
	make -C nxplib/lpc_chip_11uxx_lib clean
	make -C nxplib/nxp_lpcxpresso_11u14_board_lib clean
	rm -f $(OBJS) $(FIRMWARE).axf $(FIRMWARE).map
