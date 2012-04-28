/* ld script to make ARM BIOS
 */
OUTPUT_ARCH(arm)
ENTRY(_entry)

PHDRS
{
	text	PT_LOAD;
	data	PT_LOAD;
}

SECTIONS
{
	. = 0x30000	 ;
	.text : {
		_text = .;		/* Text and read-only data      */
		  *(.text)
		  *(.fixup)
		  *(.gnu.warning)
		  *(.rodata)
		  *(.glue_7)
		  *(.glue_7t)
		  *(.unicore16) 
		  *(.unicore32)
		. = ALIGN(4);
		_etext = .;
	} 

	.data : {			/* Data                         */
		_data = .;
		  *(.data)
		_edata = .;
	} 
	_data_sz = SIZEOF(.data);

	.bss : {
		_bss_start = .;		/* BSS                          */
		  *(.bss)
		  *(COMMON)
		_end = . ;
	} :data
}
