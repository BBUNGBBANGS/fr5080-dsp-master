    .text
    .align  4
    .global _boot_apb2spi_init
    
_boot_apb2spi_init:
    movi a4, 0x50080410
    movi a5, 4
    memw
    s32i a5, a4, 0
    movi a5, 2
    memw
    s32i a5, a4, 4
    ret

    .align  4
    .global _boot_apb2spi_read
_boot_apb2spi_read:
    movi a3, 0x50080400
    memw
    s32i a2, a3, 0xC
    movi a2, 0x01
    memw
    s32i a2, a3, 0x0
    movi a5, 2
    movi a4, 0
1:  memw
    l32i a2, a3, 0x0
    and a2, a2, a5
    beq a2, a4, 1b
    l32i a2, a3, 0x8
    ret
