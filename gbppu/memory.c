//
//  memory.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-28
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "memory.h"
#include "ppu.h"

uint8_t RAM[65536];

void
mem_init()
{
#if BUILD_USER_Lisa
	FILE *tetris = fopen("/Users/lisa/Projects/gbcpu/gbppu/tetris.gb", "r");
	FILE *file = fopen("/Users/lisa/Projects/gbcpu/gbppu/DMG_ROM.bin", "r");
#else
	FILE *tetris = fopen("/Users/mist/Documents/git/gbcpu/gbppu/tetris.gb", "r");
	FILE *file = fopen("/Users/mist/Documents/git/gbcpu/gbppu/DMG_ROM.bin", "r");
#endif

	fread(RAM, 32768, 1, tetris);
	fclose(tetris);
	fread(RAM, 256, 1, file);
	fclose(file);
}

uint8_t
mem_read(uint16_t a16)
{
	ppu_step();

	if (a16 >= 0xff00 && a16 < 0xff80) {
		extern uint8_t io_read(uint8_t);
		return io_read(a16 & 0xff);
	} else {
		return RAM[a16];
	}
}

void
mem_write(uint16_t a16, uint8_t d8)
{
	ppu_step();

	if (a16 >= 0xff00 && a16 < 0xff80) {
		extern void io_write(uint8_t, uint8_t);
		io_write(a16 & 0xff, d8);
	} else {
		RAM[a16] = d8;
	}
}
