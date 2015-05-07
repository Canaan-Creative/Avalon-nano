#
# Author: Mikeqin <Fengling.Qin@gmail.com>
#
# This is free and unencumbered software released into the public domain.
# For details see the UNLICENSE file at the root of the source tree.
#

INCLUDES += -I./inc
LIBNAME = $(shell pwd |sed 's/^\(.*\)[/]//' )
SRCS += $(wildcard src/*.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
SLIB=lib$(LIBNAME).a
LIBDIR=./libs

all: $(SLIB)

$(SLIB): ${OBJS}
	@echo "----make $(SLIB) to $(LIBDIR)----"
	@mkdir -p $(LIBDIR)
	@$(AR) -r -o $@ $(OBJS)
	-@mv $(SLIB) $(LIBDIR)

%.o : %.c
	$(CC) -c -o $@ $< $(PLATFORM_CFLAGS) $(INCLUDES)

clean:
	-@rm -f $(OBJS) $(SLIB) $(LIBDIR)/$(SLIB)
