CC := /usr/bin/gcc
USE_NVC := 1
LINUX := 1
PLAT_CFLAGS := -std=gnu99
PLAT_LDFLAGS := -Wl,-rpath=/usr/local/lib,--enable-new-dtags -Wl,-rpath=/lib64,--enable-new-dtags

-include makefile
