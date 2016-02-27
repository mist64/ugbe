//
//  ppu.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "ppu.h"
#include "ppu_private.h"

extern uint8_t RAM[];
uint8_t *reg = RAM + 0xFF00;

int current_x;
int current_y;
int oam_mode_counter;
int pixel_transfer_mode_counter;

int pixel_x, pixel_y;
uint8_t picture[160][144];

int ppu_dirty;

enum {
	mode_hblank = 0,
	mode_vblank = 1,
	mode_oam    = 2,
	mode_pixel  = 3,
} mode;

uint8_t
io_read(uint8_t a8)
{
	switch (a8) {
		case rLY:
			return current_y;
		case rSCX:
		case rSCY:
		case rBGP:
		case rOBP0:
		case rOBP1:
		case rWX:
		case rWY:
			// these behave like RAM
			return RAM[0xff00 + a8];
		default:
			printf("warning: I/O read 0xff%02x\n", a8);
			return RAM[0xff00 + a8];
	}
}

void
io_write(uint16_t a8, uint8_t d8)
{
	switch (a8) {
		// TODO: some ports may trigger something
		default:
			RAM[0xff00 + a8] = d8;
	}
}


void
ppu_init()
{
	mode = mode_oam;
	oam_mode_counter = 0;

	current_x = 0;
	current_y = 0;

	pixel_x = 0;
	pixel_y = 0;
}

int
ppu_output_pixel(uint8_t p)
{
	if (pixel_x++ < 160) {
		picture[pixel_x][pixel_y] = p;
		return 1;
	} else {
		return 0;
	}
}

void
ppu_new_line()
{
	pixel_x = 0;
	if (++pixel_y == 144) {
		pixel_y = 0;
	}
}

uint8_t
paletted(uint8_t pal, uint8_t p)
{
	return (pal >> (p * 2)) & 3;
}

char
printable(uint8_t p)
{
	switch (p) {
		default:
		case 0:
			return 'X';
		case 1:
			return 'O';
		case 2:
			return '.';
		case 3:
			return ' ';
	}
}

uint16_t vram_address;

void
vram_set_address(uint16_t addr)
{
	vram_address = addr;
}

uint8_t
vram_get_data()
{
	return RAM[vram_address];
}


void
bg_step()
{
	static uint8_t ybase;
	static uint16_t bgptr;
	static uint8_t data0;

	// background @ 2 MHz
	switch ((pixel_transfer_mode_counter / 2) & 3) {
		case 0: {
			// cycle 0: generate tile map address and prepare reading index
			uint8_t xbase = ((reg[rSCX] >> 3) + pixel_transfer_mode_counter / 8) & 31;
			ybase = reg[rSCY] + current_y;
			uint8_t ybase_hi = ybase >> 3;
			uint16_t charaddr = 0x9800 | (!!(reg[rSTAT] & LCDCF_BG9C00) << 10) | (ybase_hi << 5) | xbase;
			vram_set_address(charaddr);
			//						printf("%s:%d %04x\n", __FILE__, __LINE__, charaddr);
			break;
		}
		case 1: {
			// cycle 1: read index, generate tile data address and prepare reading tile data #0
			uint8_t index = vram_get_data();
			if (reg[rLCDC] & LCDCF_BG8000) {
				bgptr = 0x8000 + index * 16;
			} else {
				bgptr = 0x9000 + (int8_t)index * 16;
			}
			bgptr += (ybase & 7) * 2;
			vram_set_address(bgptr);
			break;
		}
		case 2: {
			// cycle 2: read tile data #0, prepare reading tile data #1
			data0 = vram_get_data();
			vram_set_address(bgptr + 1);
		}
		case 4: {
			// cycle 3: read tile data #1, output pixels
			// (VRAM is idle)
			uint8_t data1 = vram_get_data();
			int start = (pixel_transfer_mode_counter >> 3) ? 7 : (7 - (reg[rSCX] & 7));

			for (int i = start; i >= 0; i--) {
				int b0 = (data0 >> i) & 1;
				int b1 = (data1 >> i) & 1;
				//							printf("%c", printable(paletted(reg[rBGP], b0 | b1 << 1)));
				if (!ppu_output_pixel(paletted(reg[rBGP], b0 | b1 << 1))) {
					// line is full, end this
					ppu_new_line();
					mode = mode_hblank;
					break;
				}
			}
			break;
		}
	}
}

// PPU steps are executed the CPU clock rate, i.e. at ~4 MHz
void
ppu_step_4()
{
	if (current_y <= PPU_LAST_VISIBLE_LINE) {

		if (mode == mode_oam) {
			if (++oam_mode_counter == 80) {
				mode = mode_pixel;
				pixel_transfer_mode_counter = 0;
			}
		}

		if (mode == mode_pixel) {
			if (!(pixel_transfer_mode_counter & 1)) {
				bg_step();
			}
			// TODO: pixel clocking @ 4 MHz

			pixel_transfer_mode_counter++;
		}
	}

	if (++current_x == PPU_CLOCKS_PER_LINE) {
		current_x = 0;
		mode = mode_oam;
		oam_mode_counter = 0;
		if (++current_y == PPU_NUM_LINES) {
			current_y = 0;
			ppu_dirty = 1;
		}
	}
}

// This needs to be called with ~1 MHz
void
ppu_step()
{
	ppu_step_4();
	ppu_step_4();
	ppu_step_4();
	ppu_step_4();
}
