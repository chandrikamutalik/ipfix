    .global log4cfg
    .global log4cfg_size
    .section .rodata
log4cfg:
    .incbin "config/log4crc"
1:
log4cfg_size:
    .int 1b - log4cfg
