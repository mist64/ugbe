//
//  ppu.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "ppu.h"
#include "memory.h"
#include "io.h"
#include <string.h>

#undef DEBUG

#define PPU_NUM_LINES 154
#define PPU_NUM_VISIBLE_LINES 144
#define PPU_NUM_VISIBLE_PIXELS_PER_LINE 160
#define PPU_CLOCKS_PER_LINE (114*4)

#define LCDCF_ON      (1 << 7) /* LCD Control Operation */
#define LCDCF_WIN9C00 (1 << 6) /* Window Tile Map Display Select */
#define LCDCF_WINON   (1 << 5) /* Window Display */
#define LCDCF_BG8000  (1 << 4) /* BG & Window Tile Data Select */
#define LCDCF_BG9C00  (1 << 3) /* BG Tile Map Display Select */
#define LCDCF_OBJ16   (1 << 2) /* OBJ Construction */
#define LCDCF_OBJON   (1 << 1) /* OBJ Display */
#define LCDCF_BGON    (1 << 0) /* BG Display */

#pragma mark - Debug

void ppu::
debug_init()
{
	*debug_string_pixel = 0;
	*debug_string_fetch = 0;
}

void ppu::
debug_pixel(char c)
{
	char s[2];
	s[0] = c;
	s[1] = 0;
	if (strlen(debug_string_pixel) < 160) {
		strcat(debug_string_pixel, s);
	}
//	printf("%s", s);
}

void ppu::
debug_fetch(char c)
{
	char s[2];
	s[0] = c;
	s[1] = 0;
	if (strlen(debug_string_fetch) < 160) {
		strcat(debug_string_fetch, s);
	}
//	printf("%s", s);
}

void ppu::
debug_flush()
{
#ifdef DEBUG
	if (*debug_string_pixel) {
		printf("PIX:%s\n", debug_string_pixel);
		printf("FET:%s\n", debug_string_fetch);
	}
#endif
	debug_init();
}


#pragma mark - I/O

uint8_t ppu::
io_read(uint8_t a8)
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
			return _io.reg[a8];
		case rSTAT: /* 0x41 */
			return (_io.reg[a8] & 0xFC) | mode;
		case rLY:   /* 0x44 */
			return line;
	}
	assert(0);
}

void ppu::
io_write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;

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
				_memory.write(0xfe00 + i, _memory.read((d8 << 8) + i));
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


#pragma mark - VRAM & OAM

uint8_t ppu::
vram_read(uint16_t a16)
{
	if (vram_locked) {
		return 0xff;
	} else {
		return vram[a16];
	}
}

void ppu::
vram_write(uint16_t a16, uint8_t d8)
{
	if (!vram_locked) {
		vram[a16] = d8;
	}
}

uint8_t ppu::
oamram_read(uint8_t a8)
{
	if (oamram_locked) {
		return 0xff;
	} else {
		return oamram[a8];
	}
}

void ppu::
oamram_write(uint8_t a8, uint8_t d8)
{
	if (!oamram_locked) {
		oamram[a8] = d8;
	}
}


#pragma mark - Init

ppu::
ppu(memory &memory, io &io)
	: _memory(memory)
	, _io    (io)
{
	vram = (uint8_t *)calloc(0x2000, 1);
	oamram = (uint8_t *)calloc(0xa0, 1);

	clock_even = false;
	screen_off = true;
}

void ppu::
screen_reset()
{
	clock = 0;
	line = 0;
	ppicture = (uint8_t *)picture;

	debug_init();

	line_reset();

	old_mode = mode_vblank;

	oam_reset();
}


#pragma mark - VRAM R/W

void ppu::
vram_set_address(uint16_t addr)
{
	assert(vram_locked);
	vram_address = addr;
}

uint8_t ppu::
vram_get_data()
{
	assert(vram_locked);
	return vram[vram_address];
}


#pragma mark - Mode 0: H-Blank

void ppu::
hblank_reset()
{
	mode = mode_hblank;
	vram_locked = false;
	oamram_locked = false;
}

void ppu::
hblank_step()
{
	if (clock == PPU_CLOCKS_PER_LINE) {
		clock = 0;
		line_reset();
		if (++line == PPU_NUM_VISIBLE_LINES) {
			vblank_reset();
		} else {
			oam_reset();
		}
	}
}


#pragma mark - Mode 1: V-Blank

void ppu::
vblank_reset()
{
	mode = mode_vblank;
	vram_locked = false;
	oamram_locked = false;
}

void ppu::
vblank_step()
{
	if (clock == PPU_CLOCKS_PER_LINE) {
		clock = 0;
		if (++line == PPU_NUM_LINES) {
			screen_reset();
			dirty = true;
		}
	}
}


#pragma mark - Mode 2: OAM Search

uint8_t ppu::
get_sprite_height()
{
	return _io.reg[rLCDC] & LCDCF_OBJ16 ? 16 : 8;
}

void ppu::
oam_reset()
{
	mode = mode_oam;
	oam_mode_counter = 0;
	vram_locked = false;
	oamram_locked = true;
}

void ppu::
oam_step()
{
	// do all the logic in the first cycle...
	if (!oam_mode_counter && _io.reg[rLCDC] & LCDCF_OBJON) {
		oamentry *oam = (oamentry *)oamram;
		bool sprite_used[40];

		// find the 10 leftmost sprites
		uint8_t sprite_height = get_sprite_height();

		for (int i = 0; i < 40; i++) {
			sprite_used[i] = false;
		}

		sprites_visible = 0;
		int oam_index;
//		printf("collecting: ");
		do {
			uint8_t minx = 0xff;
			oam_index = -1;
			for (int i = 0; i < 40; i++) {
				int spry = oam[i].y - 16;
				if (!sprite_used[i] /* && oam[i].x */ && line >= spry && line < spry + sprite_height && oam[i].x < minx) {
					minx = oam[i].x;
					oam_index = i;
				}
			}
			if (oam_index >= 0) {
//				printf("%d/%d ", oam[oam_index].x, oam[oam_index].y);
				sprite_used[oam_index] = true;
				active_sprite_index[sprites_visible] = oam_index;
				sprites_visible++;
			}
		} while (sprites_visible < 10 && oam_index >= 0);
//		printf("\n");

		cur_sprite = 0;
	}

	// we're done after 80 cycles at ~4 MHz
	if (++oam_mode_counter == 80) {
		pixel_reset();
	}
}


#pragma mark - Mode 3: Pixel Transfer

void ppu::
pixel_reset()
{
	mode = mode_pixel;
	vram_locked = true;
	oamram_locked = true;

	bg_index_ctr = 0;
	bg_t = 0;
	window = 0;
}

void ppu::
line_reset()
{
	skip = 8 | (_io.reg[rSCX] & 7);
	pixel_x = -(_io.reg[rSCX] & 7);

	for (int i = 0; i < 16; i++) {
		bg_pixel_queue[i] = { 0, source_invalid };
	}
	bg_count = 0;
	bg_t = 0;

	debug_flush();
}

void ppu::
pixel_step()
{
	if (pixel_x == PPU_NUM_VISIBLE_PIXELS_PER_LINE + 8) {
		// we have enough pixels for the line -> end this mode
		hblank_reset();
	} else if (cur_sprite != sprites_visible && ((oamentry *)oamram)[active_sprite_index[cur_sprite]].x == pixel_x) {
		// we can't shift out pixels because a sprite starts at this position -> fetch a sprite
		debug_pixel('s');
		cur_oam = &((oamentry *)oamram)[active_sprite_index[cur_sprite]];
		line_within_tile = line - cur_oam->y + 16;
		if (cur_oam->attr & 0x40) { // Y flip
			line_within_tile = get_sprite_height() - line_within_tile - 1;
		}
		if (!fetch_is_sprite) {
			fetch_is_sprite = true;
			bg_t = 1;
		}
	} else if (!window && _io.reg[rLCDC] & LCDCF_WINON && line >= _io.reg[rWY] && pixel_x + 8 == _io.reg[rWX]) {
		// the window starts at this position -> clear pixel buffer, switch to window fetches
		debug_pixel('w');
		window = 1;
		bg_t = 0;
		bg_count = 0;
		bg_index_ctr = 0;
	} else if (!bg_count) {
		// no background/window pixels in the upper half of the queue -> wait for fetch to complete
		debug_pixel('_');
	} else {
		// output a pixel
		pixel_t pixel = bg_pixel_queue[0];
		memmove(bg_pixel_queue, bg_pixel_queue + 1, 15);
		bg_count--;
		uint8_t palette_reg = pixel.source == source_obj0 ? rOBP0 :
							  pixel.source == source_obj1 ? rOBP1 :
							  rBGP;
		if (skip) {
			// the pixel is skipped because of SCX
			debug_pixel('-');
			--skip;
		} else {
			debug_pixel(pixel.source == source_invalid ? '*' : pixel.source == source_bg ? pixel.value + '0' : pixel.value + 'A');

			if (pixel_x >= 8) {
				*ppicture++ = (_io.reg[palette_reg] >> (pixel.value << 1)) & 3;
			}
		}
		pixel_x++;
	}
}

void ppu::
fetch_step()
{
	if (mode != mode_pixel) {
		// pixel_step() has ended this mode
		return;
	}
	// the pixel transfer mode is VRAM-bound, so it runs at 1/2 speed = 2 MHz
	if (clock_even) {
		debug_fetch('-');
		return;
	}

	// background @ 2 MHz
	switch (bg_t) {
		case 0: {
		case0:
			debug_fetch('a');
			// T0: generate tile map address and prepare reading index
			uint8_t xbase;
			uint8_t ybase;
			uint8_t index_ram_select_mask;
			if (window) {
				xbase = bg_index_ctr;
				ybase = line - _io.reg[rWY];
				index_ram_select_mask = LCDCF_WIN9C00;
			} else {
				xbase = ((_io.reg[rSCX] >> 3) + bg_index_ctr) & 31;
				ybase = _io.reg[rSCY] + line;
				index_ram_select_mask = LCDCF_BG9C00;
			}
			uint8_t ybase_hi = ybase >> 3;
			line_within_tile = ybase & 7;
			uint16_t charaddr = 0x1800 | (!!(_io.reg[rLCDC] & index_ram_select_mask) << 10) | (ybase_hi << 5) | xbase;
			vram_set_address(charaddr);
			bg_t = 1;
			break;
		}
		case 1: {
			debug_fetch(fetch_is_sprite ? 'B' : 'b');
			// T1: read index, generate tile data address and prepare reading tile data #0
			uint8_t index = fetch_is_sprite ? cur_oam->tile : vram_get_data();
			if (fetch_is_sprite || (_io.reg[rLCDC] & LCDCF_BG8000)) {
				bgptr = index * 16;
			} else {
				bgptr = 0x1000 + (int8_t)index * 16;
			}
			bgptr += line_within_tile * 2;
			vram_set_address(bgptr);
			bg_t = 2;
			break;
		}
		case 2: {
			debug_fetch(fetch_is_sprite ? 'C' : 'c');
			// T2: read tile data #0, prepare reading tile data #1
			data0 = vram_get_data();
			vram_set_address(bgptr + 1);
			bg_t = 3;
			break;
		}
		case 3: {
			debug_fetch(fetch_is_sprite ? 'D' : 'd');
			// T3: read tile data #1, output pixels
			// (VRAM is idle)
			if (!fetch_is_sprite && bg_count) {
				// the right half of the pixel pipeline isn't ready yet, repeat T3
				break;
			}
			uint8_t data1 = vram_get_data();
			for (int i = 7; i >= 0; i--) {
				uint8_t value = ((data0 >> i) & 1) | (((data1 >> i) & 1) << 1);
				if (fetch_is_sprite) {
					uint8_t i2 = (cur_oam->attr & 0x20) ? i : 7 - i; // flip
					if (value && // don't draw transparent sprite pixels
						bg_pixel_queue[i2].source == source_bg && // don't draw over other sprites
						(!(cur_oam->attr & 0x80) || !bg_pixel_queue[i2].value)) { // don't draw if behind bg pixels
						bg_pixel_queue[i2] = { value, (unsigned char)(cur_oam->attr & 0x10 ? source_obj1 : source_obj0) };
					}
				} else {
					bg_pixel_queue[8 + (7 - i)] = { value, source_bg };
					bg_count++;
				}
			}
			if (fetch_is_sprite) {
				if (cur_sprite != sprites_visible) {
					cur_sprite++;
				}
				fetch_is_sprite = false;
			} else {
				bg_index_ctr++;
			}
			bg_t = 0;
			break;
		}
	}
}


#pragma mark - IRQ

void ppu::
irq_step()
{
	// V-Blank interrupt
	if (line == 144 && clock == 0) {
		_io.reg[rIF] |= 1;
	}
	// LY == LYC interrupt
	if (_io.reg[rSTAT] & 0x40 && _io.reg[rLYC] == line && clock == 0) {
		_io.reg[rIF] |= 2;
	}
	if (mode != old_mode) {
		if (_io.reg[rSTAT] & 0x20 && mode == mode_oam) {
			// Mode 2 interrupt
			_io.reg[rIF] |= 2;
		} else if (_io.reg[rSTAT] & 0x10 && mode == mode_vblank) {
			// Mode 1 interrupt
			_io.reg[rIF] |= 2;
		} else if (_io.reg[rSTAT] & 0x08 && mode == mode_hblank) {
			// Mode 0 interrupt
			_io.reg[rIF] |= 2;
		}
	}
	old_mode = mode;
}


#pragma mark - Main Logic

// PPU steps are executed the CPU clock rate, i.e. at ~4 MHz
void ppu::
step()
{
	clock_even = !clock_even;

	if ((_io.reg[rLCDC] & LCDCF_ON)) {
		if (screen_off) {
			screen_off = false;
			screen_reset();
		}
	} else {
		if (!screen_off) {
			screen_off = true;
			vblank_reset();
		}
		return;
	}

	irq_step();

	clock++;

	switch (mode) {
		case mode_oam:
			oam_step();
			break;
		case mode_pixel:
			pixel_step();
			fetch_step();
			break;
		case mode_hblank:
			hblank_step();
			break;
		case mode_vblank:
			vblank_step();
			break;
	}
}
