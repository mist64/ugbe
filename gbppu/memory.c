//
//  memory.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-28
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdlib.h>
#include "memory.h"
#include "ppu.h"

uint8_t *bootrom;
uint8_t *rom;
uint8_t *ram;
uint8_t *vram;
uint8_t *extram;
uint8_t *oamram;
uint8_t *hiram;

int bootrom_enabled;

void
mem_init()
{
	char *bootrom_filename;
	char *cartridge_filename;

#if BUILD_USER_Lisa
	bootrom_filename = "/Users/lisa/Projects/gbcpu/gbppu/tetris.gb";
	cartridge_filename = "/Users/lisa/Projects/gbcpu/gbppu/DMG_ROM.bin";
#else
	bootrom_filename = "/Users/mist/Documents/git/gbcpu/gbppu/tetris.gb";
	cartridge_filename = "/Users/mist/Documents/git/gbcpu/gbppu/DMG_ROM.bin";
#endif

	FILE *cartridge = fopen(bootrom_filename, "r");

	fseek(cartridge, 0L, SEEK_END);
	long size = ftell(cartridge);
	fseek(cartridge, 0L, SEEK_SET);
	rom = calloc(size, 1);
	fread(rom, size, 1, cartridge);
	fclose(cartridge);

	bootrom = calloc(0x100, 1);
	FILE *file = fopen(cartridge_filename, "r");
	fread(bootrom, 0x100, 1, file);
	fclose(file);

	ram = calloc(0x2000, 1);
	vram = calloc(0x2000, 1);
	oamram = calloc(0x90, 1);
	hiram = calloc(0x7f, 1);

	uint16_t extramsize;
	switch (rom[0x149]) {
		default:
		case 0:
			// TODO: MBC2
			extramsize = 0;
			break;
		case 1:
			extramsize = 0x800;
			break;
		case 2:
			extramsize = 0x2000;
			break;
		case 3:
			extramsize = 0x8000;
			break;
	}
	extram = calloc(extramsize, 1);

	bootrom_enabled = 1;
}

uint8_t
mem_read(uint16_t a16)
{
	ppu_step();

	if (a16 < 0x8000) {
		if (bootrom_enabled && a16 < 0x100) {
			return bootrom[a16];
		} else {
			return rom[a16];
		}
	} else if (a16 <= 0xa000) {
		return vram[a16 - 0x8000];
	} else if (a16 <= 0xc000) {
		return extram[a16 - 0xa000];
	} else if (a16 <= 0xe000) {
		return ram[a16 - 0xc000];
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		return oamram[a16 - 0xfe00];
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		extern uint8_t io_read(uint8_t);
		return io_read(a16 & 0xff);
	} else if (a16 >= 0xff80) {
		return hiram[a16 - 0xff80];
	} else {
		printf("warning: read from 0x%04x!\n", a16);
		return 0;
	}
}

void
mem_write(uint16_t a16, uint8_t d8)
{
	ppu_step();

	if (a16 < 0x8000) {
		// TODO: MBC
	} else if (a16 <= 0xa000) {
		vram[a16 - 0x8000] = d8;
	} else if (a16 <= 0xc000) {
		extram[a16 - 0xa000] = d8;
	} else if (a16 <= 0xe000) {
		ram[a16 - 0xc000] = d8;
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		oamram[a16 - 0xfe00] = d8;
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		extern void io_write(uint8_t, uint8_t);
		io_write(a16 & 0xff, d8);
	} else if (a16 >= 0xff80) {
		hiram[a16 - 0xff80] = d8;
	} else {
		printf("warning: write to 0x%04x!\n", a16);
	}
}

void
disable_bootrom()
{
	bootrom_enabled = 0;
}

uint8_t
vram_read(uint16_t a16)
{
	return vram[a16 - 0x8000];
}
