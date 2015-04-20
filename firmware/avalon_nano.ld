GROUP(
	libcr_nohost.a
	libcr_c.a
	libcr_eabihelpers.a
)

MEMORY
{
	/* Define each memory region */
	MFlash32 (rx) : ORIGIN = 0x0, LENGTH = 0x8000 /* 32K bytes */
	RamLoc4 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x1000 /* 4K bytes */
	RamUsb2 (rwx) : ORIGIN = 0x20004000, LENGTH = 0x800 /* 2K bytes */
}

/* Define a symbol for the top of each memory region */
__top_MFlash32 = 0x0 + 0x8000;
__top_RamLoc4 = 0x10000000 + 0x1000;
__top_RamUsb2 = 0x20004000 + 0x800;

ENTRY(ResetISR)

SECTIONS
{
	/* MAIN TEXT SECTION */
	.text : ALIGN(4)
	{
		FILL(0xff)
		__vectors_start__ = ABSOLUTE(.);
		KEEP(*(.isr_vector))

		/* Global Section Table */
		. = ALIGN(4);
		__section_table_start = .;
		__data_section_table = .;
		LONG(LOADADDR(.data));
		LONG(ADDR(.data));
		LONG(SIZEOF(.data));
		LONG(LOADADDR(.data_RAM2));
		LONG(ADDR(.data_RAM2));
		LONG(SIZEOF(.data_RAM2));
		__data_section_table_end = .;
		__bss_section_table = .;
		LONG(ADDR(.bss));
		LONG(SIZEOF(.bss));
		LONG(ADDR(.bss_RAM2));
		LONG(SIZEOF(.bss_RAM2));
		__bss_section_table_end = .;
		__section_table_end = . ;
		/* End of Global Section Table */

		*(.after_vectors*)

		/* Code Read Protect data */
		. = 0x000002FC;
		PROVIDE(__CRP_WORD_START__ = .);
		KEEP(*(.crp))
		PROVIDE(__CRP_WORD_END__ = .);
		ASSERT(!(__CRP_WORD_START__ == __CRP_WORD_END__), "Linker CRP Enabled, but no CRP_WORD provided within application");
		/* End of Code Read Protect */
	} >MFlash32

	.text : ALIGN(4)
	{
		*(.text*)
		*(.rodata .rodata.* .constdata .constdata.*)
		. = ALIGN(4);
	} > MFlash32

	/*
	 * for exception handling/unwind - some Newlib functions (in common
	 * with C++ and STDC++) use this.
	 */
	.ARM.extab : ALIGN(4)
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > MFlash32
	__exidx_start = .;

	.ARM.exidx : ALIGN(4)
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > MFlash32
	__exidx_end = .;

	_etext = .;

	/* DATA section for RamUsb2 */
	.data_RAM2 : ALIGN(4)
	{
		FILL(0xff)
		*(.ramfunc.$RAM2)
		*(.ramfunc.$RamUsb2)
		*(.data.$RAM2*)
		*(.data.$RamUsb2*)
		. = ALIGN(4);
	} > RamUsb2 AT>MFlash32

	/* MAIN DATA SECTION */
	.uninit_RESERVED : ALIGN(4)
	{
		KEEP(*(.bss.$RESERVED*))
		. = ALIGN(4);
		_end_uninit_RESERVED = .;
	} > RamLoc4

	/* Main DATA section (RamLoc4) */
	.data : ALIGN(4)
	{
		FILL(0xff)
		_data = .;
		*(vtable)
		*(.ramfunc*)
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	    } > RamLoc4 AT>MFlash32

	    /* BSS section for RamUsb2 */
	    .bss_RAM2 : ALIGN(4)
	    {
		    *(.bss.$RAM2*)
		    *(.bss.$RamUsb2*)
		    . = ALIGN(4);
	} > RamUsb2

	/* MAIN BSS SECTION */
	.bss : ALIGN(4)
	{
		_bss = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4) ;
		_ebss = .;
		PROVIDE(end = .);
	} > RamLoc4

	/* NOINIT section for RamUsb2 */
	.noinit_RAM2 (NOLOAD) : ALIGN(4)
	{
		*(.noinit.$RAM2*)
		*(.noinit.$RamUsb2*)
		. = ALIGN(4);
	} > RamUsb2

	/* DEFAULT NOINIT SECTION */
	.noinit (NOLOAD): ALIGN(4)
	{
		_noinit = .;
		*(.noinit*)
		. = ALIGN(4) ;
		_end_noinit = .;
	} > RamLoc4

	PROVIDE(_pvHeapStart = .);
	PROVIDE(_vStackTop = __top_RamLoc4 - 0);
}
