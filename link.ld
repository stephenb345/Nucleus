ENTRY(main)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ;
    rodata  PT_LOAD    FLAGS((1 << 2)) ;
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ;
    dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2)) ;
}
 
SECTIONS
{
    . = 2M;
    kernel_start = .;

    .text : {
        *(.text .text.*)
    } :text
    . += CONSTANT(MAXPAGESIZE);
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata
    . += CONSTANT(MAXPAGESIZE);
    .data : {
        *(.data .data.*)
    } :data
    .dynamic : {
        *(.dynamic)
    } :data :dynamic
    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }

    kernel_end = .;
}