CFLAGS = -I../nxp_lpcxpresso_11u14_usbd_lib_cdc_uart/example/inc

help:
	@echo "usage"
	@echo "make conv/convclean----compile/clean conv test data"
	@echo "make chk/chkclean ----compile/clean the avalon usb data chk utility"
	@echo "make help----print this list"

conv:
	gcc -o convdata convdata.c

convclean:
	rm convdata

chk:
	gcc -g -o chkdata chk_avalon_usbdata.c ../nxp_lpcxpresso_11u14_usbd_lib_cdc_uart/example/src/sha2.c $(CFLAGS) 
chkclean:
	rm chkdata

