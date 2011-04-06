CC = gcc

CFLAGS = -lusb-1.0

ifeq ($(ARCH),mac)
CFLAGS += -framework CoreFoundation -framework IOKit -I./include
endif

ifeq ($(DEVICE),a4)
$(shell echo ".set RET_ADDR, 0x7ef" > device_def.S)
DEVICE_DEF = -DDEVICE_A4
endif

ifeq ($(DEVICE),3g)
$(shell echo ".set RET_ADDR, 0x8b7" > device_def.S)
DEVICE_DEF = -DDEVICE_3G
endif

ifeq ($(DEVICE),3gs_new_bootrom)
$(shell echo ".set RET_ADDR, 0x8b7" > device_def.S)
DEVICE_DEF = -DDEVICE_3GS_NEW_BOOTROM
endif

ifndef DEVICE_DEF
$(error You must define the device type by specifying DEVICE=< a4 | 3g | 3gs_new_bootrom > in your make invocation)
endif

all:
		$(CC) bdu.c -o bdu $(CFLAGS) $(DEVICE_DEF)
		
		arm-elf-as -mthumb --fatal-warnings -o payload.o payload.S
		arm-elf-objcopy -O binary  payload.o payload.bin
		rm payload.o device_def.S

