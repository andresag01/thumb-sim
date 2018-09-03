# MIT License
#
# Copyright (c) 2018 Andres Amaya Garcia
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Compilers
ARMGNU ?= arm-none-eabi

# Sorce file names
CSRCS     = main.c
ASMSRCS   = startup.asm
LDSCRIPT  = memmap.ld

# Compiler options
OPT_LEVEL    ?= 2
CFLAGS       ?= -O$(OPT_LEVEL) -mcpu=cortex-m0 -mthumb \
				-fno-short-enums -nostdlib -ffreestanding -mfloat-abi=soft \
				-ffunction-sections -fdata-sections -fno-builtin \
				-fno-stack-protector -fomit-frame-pointer -DNDEBUG
LDFLAGS      ?= -nostdlib -static --gc-sections -T $(LDSCRIPT)
ASFLAGS      ?= -mthumb
READELFFLAGS ?= -a -teg
OBJDUMPFLAGS ?= -h -C -S -s -d
SIZEFLAGS    ?= --format=sysv --radix=10

# Output files
OUTPUTFILES   = example.bin       \
				example.size      \
				example.objdump   \
			  	example.readelf

OBJS = $(ASMSRCS:.asm=.o) $(CSRCS:.c=.o)

all: $(OUTPUTFILES)

# Command to generate assembly code output using llvm
%.objdump: %.elf
	@echo "    ODUMP $< -> $@"
	@$(ARMGNU)-objdump $(OBJDUMPFLAGS) $< > $@

%.readelf: %.elf
	@echo "    RELF  $< -> $@"
	@$(ARMGNU)-readelf $(READELFFLAGS) $< > $@

%.size: %.elf
	@echo "    SIZE  $< -> $@"
	@$(ARMGNU)-size $(SIZEFLAGS) $< > $@

%.o: %.c
	@echo "    CC    $< -> $@"
	@$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "    ASM   $< -> $@"
	@$(ARMGNU)-as $(ASFLAGS) $< -o $@

%.elf: $(OBJS) $(LDSCRIPT)
	@echo "    LD    $< -> $@"
	@$(ARMGNU)-ld $(LDFLAGS) $(OBJS) -o $@

%.bin: %.elf
	@echo "    OCPY  $< -> $@"
	@$(ARMGNU)-objcopy $< -O binary $@

.PRECIOUS: %.elf %.o %.readelf %.size %.objdump

.PHONY: clean all

# Clean a single example
clean:
	$(RM)   *.elf *.o *.bin *.objdump *.readelf *.size
