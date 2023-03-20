/* This linker script generated from xt-genldscripts.tpp for LSP min-rt-local */
/* Linker Script for default link */

MEMORY
{
  flash_seg :                  	    org = 0x20000000, len = 0x100000
  dram_seg :                       	org = 0x00183000, len = 0x5c000
  iram_seg :						org = 0x000a3500, len = 0x4b00
}
PHDRS
{
  flash_phdr PT_LOAD;
  dram_phdr PT_LOAD;
  iram_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(app_entry)

PROVIDE(_rodata_load_addr = LOADADDR(.rodata));
PROVIDE(_data_load_addr = LOADADDR(.data));
PROVIDE(_test_data_load_addr = LOADADDR(.test_data));
PROVIDE(_iram_text_load_addr = LOADADDR(.iram_text));

SECTIONS
{
  .entry_table : ALIGN(4)
  {
    _entry_table_start = ABSOLUTE(.);
    KEEP(*(entry_point_section))
  } >flash_seg :flash_phdr
  
  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(EXCLUDE_FILE (*libHiFi3_VFPU_Library.a:*.o) .literal)
    *(EXCLUDE_FILE (*libHiFi3_VFPU_Library.a:*.o) .text)
    *(.literal.*)
    *(.text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    KEEP(*(.keep_in_rom_code.literal))
    KEEP(*(.keep_in_rom_code))
    *(.gnu.version)
	
	/* rt-thread finsh shell */
	. = ALIGN(4);
	__fsymtab_start = .;
	KEEP(*(FSymTab))
	__fsymtab_end = .;
	. = ALIGN(4);
	__vsymtab_start = .;
	KEEP(*(VSymTab))
	__vsymtab_end = .;
	
	/* rt-thread init code */
	. = ALIGN(4);
	__rt_init_start = .;
	KEEP(*(SORT(.rti_fn*)))
	__rt_init_end = .;
	
	/* rt-thread modules */
	. = ALIGN(4);
	__rtmsymtab_start = .;
	KEEP(*(RTMSymTab))
	__rtmsymtab_end = .;
	
    . = ALIGN (4);
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >flash_seg :flash_phdr
  
  .resource : ALIGN(4)
  {
	_sresource = .;
	_resource_start = ABSOLUTE(.);
	*(resource_section)
	
	. = ALIGN (4);
	_resource_end = ABSOLUTE(.);
	_eresource = .;
  } >flash_seg :flash_phdr

  .rodata : AT(LOADADDR(.resource) + (_resource_end - _resource_start)) ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    LONG(_data_start);
    LONG(_data_end);
    LONG(LOADADDR(.data));
    LONG(0);
    LONG(0);
    LONG(0);
    
    . = ALIGN (4);
    _rodata_end = ABSOLUTE(.);
  } >dram_seg :dram_phdr

  .data : AT(LOADADDR(.rodata) + (_rodata_end - _rodata_start)) ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    . = ALIGN (4);
    _data_end = ABSOLUTE(.);
  } >dram_seg :dram_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.dram0.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_dram0_0_end = ALIGN(0x8);
  } >dram_seg :dram_phdr
  
  .test_data : AT(LOADADDR(.data) + (_data_end - _data_start)) ALIGN(4)
  {
    . = ALIGN (4);
    _test_data_start = ABSOLUTE(.);
    *(test_data)
    _test_data_end = ABSOLUTE(.);
  } >dram_seg :dram_phdr
  
  .iram_text : AT(LOADADDR(.test_data) + (_test_data_end - _test_data_start)) ALIGN(4)
  {
    . = ALIGN (4);
    _iram_text_start = ABSOLUTE(.);
    *(iram_section iram_section.literal)
    *(.iram_section.literal .iram_section)
    *libHiFi3_VFPU_Library.a:*.o (.literal .text)
    _iram_text_end = ABSOLUTE(.);
  } > iram_seg : iram_phdr

  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}

