CC = gcc
CFLAGS_OSX = -lusb-1.0 -framework CoreFoundation -framework IOKit -I./include
CFLAGS_LNX =  -lusb-1.0

all:
		@echo 'ERROR: no platform defined.'
		@echo 'LINUX USERS: make linux'
		@echo 'MAC OS X USERS: make macosx'
	
macosx:
		$(CC) bdu.c -o bdu $(CFLAGS_OSX)
		
		arm-elf-as -mthumb --fatal-warnings -o payload.o payload.S
		arm-elf-objcopy -O binary  payload.o payload.bin
		rm payload.o

linux:
		$(CC) bdu.c -o bdu $(CFLAGS_LNX)
		
		arm-elf-as -mthumb --fatal-warnings -o payload.o payload.S
		arm-elf-objcopy -O binary  payload.o payload.bin
		rm payload.o
