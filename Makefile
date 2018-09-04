CC = xtensa-lx106-elf-gcc

TOOLCHAIN ?= ../esp-open-sdk
ESP_RTOS_BASE ?= ../esp-open-rtos

SDK_BASE = $(TOOLCHAIN)/sdk
# ESP_SDK = $(shell $(CC) -print-sysroot)/usr

#SPI flash size, in KB
ESP_FLASH_SIZE ?= 4096
#0: QIO, 1: QOUT, 2: DIO, 3: DOUT
ESP_FLASH_MODE ?= 0
#0: 40MHz, 1: 26MHz, 2: 20MHz, 15: 80MHz
ESP_FLASH_FREQ_DIV ?= 15
ESP_SPEED ?= 115200
ESP_PORT ?= /dev/ttyUSB0
ESP_IP ?= 192.168.4.1

#Tag for OTA images. 0-27 characters. Change to eg your projects title.
LIBESPHTTPD_OTA_TAGNAME ?= esp_ws2812

LIBESPHTTPD_MAX_CONNECTIONS ?= 8
LIBESPHTTPD_STACKSIZE ?= 2048

CURL ?= curl
CURL_OPTS = --connect-timeout 3 --max-time 60 -s

ESPBAUD = $(ESP_SPEED)

maplookup = $(patsubst $(strip $(1)):%,%,$(filter $(strip $(1)):%,$(2)))

# CONFIG_POS = "$$(printf "0x%X" $$(($(ESP_FLASH_SIZE)*512-0x2000)))"
#CONFIG_POS = 0x1FE000
CONFIG_POS = $(call maplookup,$(ESP_FLASH_SIZE),4096:0xFE000)

#############################################################
PROGRAM=esp_ws2812

PROGRAM_SRC_DIR=src src/web
PROGRAM_INC_DIR=src/include src/include/web

PROGRAM_CFLAGS += -DFREERTOS -DLIBESPHTTPD_OTA_TAGNAME="\"$(LIBESPHTTPD_OTA_TAGNAME)\"" -DFLASH_SIZE=$(FLASH_SIZE)
PROGRAM_CFLAGS += -std=gnu99

#EXTRA_CFLAGS += -DCONFIG_POS=$(CONFIG_POS) -DFREERTOS -mlongcalls #
EXTRA_CFLAGS += -DMEMP_NUM_NETCONN=$(LIBESPHTTPD_MAX_CONNECTIONS)

EXTRA_COMPONENTS = extras/rboot-ota extras/dhcpserver extras/libesphttpd
EXTRA_COMPONENTS += extras/i2s_dma extras/ws2812_i2s

include $(ESP_RTOS_BASE)/common.mk

