# ---------------------------------------------------------------------------
# Makefile - DrawaBattle GBA
# Requer devkitPro com o pacote gba-dev instalado (devkitARM + libgba).
# Defina a variável de ambiente DEVKITARM antes de rodar `make`, ex:
#   export DEVKITARM=/opt/devkitpro/devkitARM
# ---------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM não está definido. Instale devkitPro (dkp-pacman -S gba-dev) e exporte DEVKITARM)
endif

# DEVKITPRO (pasta que contém libgba/) normalmente já vem definida pelo
# instalador/imagem devkitPro; se não vier, assume o padrão (pai de devkitARM).
DEVKITPRO ?= $(abspath $(DEVKITARM)/..)

TARGET   := drawabattle
BUILD    := build
SOURCES  := source

PREFIX   := $(DEVKITARM)/bin/arm-none-eabi-
CC       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy
GBAFIX   := $(DEVKITARM)/bin/gbafix

LIBGBA   := $(DEVKITPRO)/libgba

ARCH     := -mthumb-interwork -mthumb
CFLAGS   := -g -Wall -O2 -mcpu=arm7tdmi -mtune=arm7tdmi $(ARCH) -fomit-frame-pointer -I$(LIBGBA)/include
LDFLAGS  := -g $(ARCH) -specs=gba.specs -L$(LIBGBA)/lib

SRCS := $(wildcard $(SOURCES)/*.c)
OBJS := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET).gba

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SOURCES)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ -lgba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba
