//
//  ppu.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>
#include "ppu.h"
#include "ppu_private.h"
#include "memory.h"
#include "io.h"

uint8_t *vram;
uint8_t *oamram;

int current_x;
int current_y;
int oam_mode_counter;
int pixel_transfer_mode_counter;

int screen_off;
int vram_locked;
int oamram_locked;

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
ppu_io_read(uint8_t a8)
{
	switch (a8) {
		case rLCDC: /* 0x40 */
		case rSCY:  /* 0x42 */
		case rSCX:  /* 0x43 */
		case rLYC:  /* 0x45 */
		case rDMA:  /* 0x46 */
		case rBGP:  /* 0x47 */
		case rOBP0: /* 0x48 */
		case rOBP1: /* 0x49 */
		case rWY:   /* 0x4A */
		case rWX:   /* 0x4B */
			return io[a8];
		case rSTAT: /* 0x41 */
			return (io[a8] & 0xFC) | mode;
		case rLY:   /* 0x44 */
			return current_y;
	}
	assert(0);
}

void
ppu_io_write(uint8_t a8, uint8_t d8)
{
	io[a8] = d8;

	switch (a8) {
		case rLCDC: /* 0x40 */
		case rSTAT: /* 0x41 */
		case rSCY:  /* 0x42 */
		case rSCX:  /* 0x43 */
		case rLY:   /* 0x44 */
		case rLYC:  /* 0x45 */
			break;
		case rDMA: {/* 0x46 */
			// TODO this should take a while and block access
			// to certain memory areas
			for (uint8_t i = 0; i < 160; i++) {
				mem_write(0xfe00 + i, mem_read((d8 << 8) + i));
			}
			break;
		}
		case rBGP:  /* 0x47 */
		case rOBP0: /* 0x48 */
		case rOBP1: /* 0x49 */
		case rWY:   /* 0x4A */
		case rWX:   /* 0x4B */
			break;
		default:
			printf("warning: ppu I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);
			break;
	}
}

uint8_t
ppu_vram_read(uint16_t a16)
{
	if (vram_locked) {
		return 0xff;
	} else {
		return vram[a16];
	}
}

void
ppu_vram_write(uint16_t a16, uint8_t d8)
{
	if (!vram_locked) {
		vram[a16] = d8;
	}
}

uint8_t
ppu_oamram_read(uint8_t a8)
{
	if (oamram_locked) {
		return 0xff;
	} else {
		return oamram[a8];
	}
}

void
ppu_oamram_write(uint8_t a8, uint8_t d8)
{
	if (!oamram_locked) {
		oamram[a8] = d8;
	}
}


static void
new_screen()
{
	mode = mode_oam;
	oam_mode_counter = 0;
	vram_locked = 0;
	oamram_locked = 1;

	current_x = 0;
	current_y = 0;

	pixel_x = 0;
	pixel_y = 0;
}

void
ppu_init()
{
	vram = calloc(0x2000, 1);
	oamram = calloc(0xa0, 1);

	new_screen();
}

uint8_t oam_get_pixel(uint8_t x);

int
ppu_output_pixel(uint8_t p)
{
	if (++pixel_x < 160) {
		uint8_t p2 = oam_get_pixel(pixel_x);
		if (p2 != 255) {
			p = p2;
		}
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

static uint16_t vram_address;

void
vram_set_address(uint16_t addr)
{
	vram_address = addr;
}

uint8_t
vram_get_data()
{
	return vram[vram_address];
}

#pragma pack(1)
typedef struct {
	uint8_t y;
	uint8_t x;
	uint8_t tile;
	uint8_t attr;
} oamentry;

struct {
	oamentry oam;
	uint8_t data0;
	uint8_t data1;
} spritegen[10];

uint8_t sprites_visible;

uint8_t
oam_get_pixel(uint8_t x)
{
	uint8_t p = 255;
	if (sprites_visible) {
		for (int j = 0; j < sprites_visible; j++) {
			//				printf("line: %d; sprit e %d: x=%d, y=%d, index=%d\n", current_y, j, spritegen[j].oam.x, spritegen[j].oam.y, spritegen[j].oam.tile);
			uint8_t sprx = spritegen[j].oam.x - 8;
			if (x >= sprx && x <= sprx + 8) {
				uint8_t i = x - sprx;
				if (!(spritegen[j].oam.attr & 0x20)) { // X flip
					i = 7 - i;
				}
				int b0 = (spritegen[j].data0 >> i) & 1;
				int b1 = (spritegen[j].data1 >> i) & 1;
				uint8_t p2 = b0 | (b1 << 1);
				if (p2) {
					p = p2;
					break;
				}
//				printf("XXX: %d; sprite %d: x=%d, y=%d, index=%d, data=%02x/%02x -> p=%d\n", current_y, j, spritegen[j].oam.x, spritegen[j].oam.y, spritegen[j].oam.tile, spritegen[j].data0, spritegen[j].data1, i);
			}
		}
	}
	return p;
}

void
oam_step()
{
	oamentry *oam = (oamentry *)oamram;
	int sprite_used[40];

//	for (int i = 0; i < 160; i++) {
//		printf("%x ", oamram[i]);
//	}
//	printf("\n");

	// do all the logic in the first cycle...
	if (!oam_mode_counter && io_read(rLCDC) & LCDCF_OBJON) {
		// fill the 10 sprite generators with the 10 leftmost sprites
		uint8_t sprite_height = io_read(rLCDC) & LCDCF_OBJ16 ? 16 : 8;

		for (int i = 0; i < 40; i++) {
			sprite_used[i] = 0;
		}

		sprites_visible = 0;
		int oam_index;
		do {
			uint8_t minx = 255;
			oam_index = -1;
			for (int i = 0; i < 40; i++) {
				uint8_t spry = oam[i].y - 16;
				if (!sprite_used[i] && oam[i].x && current_y >= spry && current_y < spry + sprite_height && oam[i].x < minx) {
					oam_index = i;
				}
			}
			if (oam_index >= 0) {
				minx = oam[oam_index].x;
				sprite_used[oam_index] = 1;
				spritegen[sprites_visible].oam = oam[oam_index];
				sprites_visible++;
			}
		} while (sprites_visible < 10 && oam_index >= 0);

		if (sprites_visible) {
			for (int j = 0; j < sprites_visible; j++) {
//				printf("line: %d; sprit e %d: x=%d, y=%d, index=%d\n", current_y, j, spritegen[j].oam.x, spritegen[j].oam.y, spritegen[j].oam.tile);
				uint8_t i = ((current_y - spritegen[j].oam.y) & (sprite_height - 1));
				if (spritegen[j].oam.attr & 0x40) { // Y flip
					i = 7 - i;
				}
				uint16_t address = 16 * spritegen[j].oam.tile + i * 2;
				spritegen[j].data0 = vram[address];
				spritegen[j].data1 = vram[address + 1];
//				printf("line: %d; sprite %d: x=%d, y=%d, index=%d, address=%04x data=%02x/%02x\n", current_y, j, spritegen[j].oam.x, spritegen[j].oam.y, spritegen[j].oam.tile, address, spritegen[j].data0, spritegen[j].data1);
			}
		}
	}
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
			uint8_t xbase = ((io[rSCX] >> 3) + pixel_transfer_mode_counter / 8) & 31;
			ybase = io[rSCY] + current_y;
			uint8_t ybase_hi = ybase >> 3;
			uint16_t charaddr = 0x1800 | (!!(io[rSTAT] & LCDCF_BG9C00) << 10) | (ybase_hi << 5) | xbase;
			vram_set_address(charaddr);
			//						printf("%s:%d %04x\n", __FILE__, __LINE__, charaddr);
			break;
		}
		case 1: {
			// cycle 1: read index, generate tile data address and prepare reading tile data #0
			uint8_t index = vram_get_data();
			if (io[rLCDC] & LCDCF_BG8000) {
				bgptr = index * 16;
			} else {
				bgptr = 0x1000 + (int8_t)index * 16;
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
			int start = (pixel_transfer_mode_counter >> 3) ? 7 : (7 - (io[rSCX] & 7));

			for (int i = start; i >= 0; i--) {
				int b0 = (data0 >> i) & 1;
				int b1 = (data1 >> i) & 1;
				if (!ppu_output_pixel(paletted(io[rBGP], b0 | b1 << 1))) {
					// line is full, end this
					ppu_new_line();
					mode = mode_hblank;
					vram_locked = 0;
					oamram_locked = 0;
					break;
				}
			}
			break;
		}
	}
}

// PPU steps are executed the CPU clock rate, i.e. at ~4 MHz
void
ppu_step()
{
	if ((io[rLCDC] & LCDCF_ON)) {
		if (screen_off) {
			screen_off = 0;
			vram_locked = 0;
			oamram_locked = 0;
			new_screen();
		}
	} else {
		screen_off = 1;
	}

	if (screen_off) {
		return;
	}

	// IRQ
	if (current_y == 144 && current_x == 0) {
		io[rIF] |= 1;
	} else {
		if (io_read(rSTAT) & 0x40 && io_read(rLYC) == current_y && current_x == 0) {
			io[rIF] |= 2;
		}
	}

	if (current_y <= PPU_LAST_VISIBLE_LINE) {

		if (mode == mode_oam) {
			oam_step();
			if (++oam_mode_counter == 80) {
				mode = mode_pixel;
				pixel_transfer_mode_counter = 0;
				vram_locked = 1;
				oamram_locked = 1;
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
//		printf("\n");
		if (++current_y == PPU_NUM_LINES) {
			current_y = 0;
			ppu_dirty = 1;
//			printf("\n");
		}
		if (current_y <= PPU_LAST_VISIBLE_LINE) {
			mode = mode_oam;
			oam_mode_counter = 0;
			vram_locked = 0;
			oamram_locked = 1;
		} else {
			mode = mode_vblank;
		}
	}
}
