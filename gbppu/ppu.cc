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

#define PPU_NUM_LINES 154
#define PPU_CLOCKS_PER_LINE (114*4)
#define PPU_LAST_VISIBLE_LINE 143

#define LCDCF_ON      (1 << 7) /* LCD Control Operation */
#define LCDCF_WIN9C00 (1 << 6) /* Window Tile Map Display Select */
#define LCDCF_WINON   (1 << 5) /* Window Display */
#define LCDCF_BG8000  (1 << 4) /* BG & Window Tile Data Select */
#define LCDCF_BG9C00  (1 << 3) /* BG Tile Map Display Select */
#define LCDCF_OBJ16   (1 << 2) /* OBJ Construction */
#define LCDCF_OBJON   (1 << 1) /* OBJ Display */
#define LCDCF_BGON    (1 << 0) /* BG Display */

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
			return current_y;
	}
	assert(0);
}

void
ppu::io_write(uint8_t a8, uint8_t d8)
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

uint8_t
ppu::vram_read(uint16_t a16)
{
	if (vram_locked) {
		return 0xff;
	} else {
		return vram[a16];
	}
}

void
ppu::vram_write(uint16_t a16, uint8_t d8)
{
	if (!vram_locked) {
		vram[a16] = d8;
	}
}

uint8_t
ppu::oamram_read(uint8_t a8)
{
	if (oamram_locked) {
		return 0xff;
	} else {
		return oamram[a8];
	}
}

void
ppu::oamram_write(uint8_t a8, uint8_t d8)
{
	if (!oamram_locked) {
		oamram[a8] = d8;
	}
}


void
ppu::new_screen()
{
	mode = mode_oam;
	oam_mode_counter = 0;
	vram_locked = 0;
	oamram_locked = 1;

	current_x = 0;
	current_y = 0;

	pixel_x = 0;
	pixel_y = 0;

	bg_pixel_queue_next = 0;
	memset(oam_pixel_queue, 0xff, sizeof(oam_pixel_queue));
}

ppu::ppu(memory &memory, io &io)
	: _memory(memory)
	, _io    (io)
{
	vram = (uint8_t *)calloc(0x2000, 1);
	oamram = (uint8_t *)calloc(0xa0, 1);

	old_mode = mode_hblank;

	new_screen();
}

int ppu::
output_pixel(uint8_t p)
{
	if (pixel_x >= 160) {
		return 0;
	} else {
		uint8_t p2 = oam_get_pixel(pixel_x);
		if (p2 != 255) {
			p = p2;
		}
		picture[pixel_y][pixel_x] = p;
		pixel_x++;
		return 1;
	}
}

void ppu::
new_line()
{
	pixel_x = 0;
	if (++pixel_y == 144) {
		pixel_y = 0;
	}
}

uint8_t ppu::
paletted(uint8_t pal, uint8_t p)
{
	return (pal >> (p * 2)) & 3;
}

void ppu::
vram_set_address(uint16_t addr)
{
	vram_address = addr;
}

uint8_t ppu::
vram_get_data()
{
	return vram[vram_address];
}

uint8_t ppu::
oam_get_pixel(uint8_t x)
{
	return oam_pixel_get();
}

uint8_t ppu::
get_sprite_height()
{
	return _io.reg[rLCDC] & LCDCF_OBJ16 ? 16 : 8;
}

void ppu::
oam_step()
{
	oamentry *oam = (oamentry *)oamram;
	int sprite_used[40];

	// do all the logic in the first cycle...
	if (!oam_mode_counter && _io.reg[rLCDC] & LCDCF_OBJON) {
		// fill the 10 sprite generators with the 10 leftmost sprites
		uint8_t sprite_height = get_sprite_height();

		for (int i = 0; i < 40; i++) {
			sprite_used[i] = 0;
		}

		sprites_visible = 0;
		int oam_index;
//		printf("collecting: ");
		do {
			uint8_t minx = 0xff;
			oam_index = -1;
			for (int i = 0; i < 40; i++) {
				uint8_t spry = oam[i].y - 16;
				if (!sprite_used[i] && oam[i].x && current_y >= spry && current_y < spry + sprite_height && oam[i].x < minx) {
					minx = oam[i].x;
					oam_index = i;
				}
			}
			if (oam_index >= 0) {
				sprite_used[oam_index] = 1;
				spritegen[sprites_visible].oam = oam[oam_index];
//				printf("%d/%d ", spritegen[sprites_visible].oam.x, spritegen[sprites_visible].oam.y);
				sprites_visible++;
			}
		} while (sprites_visible < 10 && oam_index >= 0);
//		printf("\n");

		cur_sprite = 0;
	}

	if (++oam_mode_counter == 80) {
		bg_reset();
	}
}

void ppu::
bg_reset()
{
	mode = mode_pixel;
	vram_locked = 1;
	oamram_locked = 1;

	bg_index_ctr = 0;
	bg_t = 0;
	window = 0;
}

void ppu::
bg_pixel_push(uint8_t p)
{
	bg_pixel_queue[bg_pixel_queue_next++] = p;
	assert(bg_pixel_queue_next <= sizeof(bg_pixel_queue));
}

uint8_t ppu::
bg_pixel_get()
{
	if (!bg_pixel_queue_next) {
		return 0xff;
	}
	uint8_t p = bg_pixel_queue[0];
	memmove(bg_pixel_queue, bg_pixel_queue + 1, --bg_pixel_queue_next);
	return p;
}

void ppu::
oam_pixel_set(int i, uint8_t p)
{
#if 0 // TODO: this sometimes happens, needs to be debugged
	assert(i < sizeof(oam_pixel_queue));
#else
	if (!(i < sizeof(oam_pixel_queue))) {
		return;
	}
#endif
	if (oam_pixel_queue[i] == 0xff) {
		oam_pixel_queue[i] = p;
	}
}

uint8_t ppu::
oam_pixel_get()
{
	uint8_t total = 0xff;
	for (int i = 0; i < sizeof(oam_pixel_queue); i++) {
		total &= oam_pixel_queue[i];
	}
	if (total != 0xff) {
//		printf("oam_pixel_queue: ");
		for (int i = 0; i < sizeof(oam_pixel_queue); i++) {
//			printf("%02x ", oam_pixel_queue[i]);
		}
//		printf("\n");
	}
	uint8_t p = oam_pixel_queue[0];
	memmove(oam_pixel_queue, oam_pixel_queue + 1, sizeof(oam_pixel_queue) - 1);
	return p;
}


void ppu::
bg_step()
{
	// background @ 2 MHz
	switch (bg_t) {
		case 0: {
			// decide whether we should do a sprite tile fetch here
			if (cur_sprite != sprites_visible &&
				   (((spritegen[cur_sprite].oam.x >> 3) - 2) == (pixel_x >> 3) || (spritegen[cur_sprite].oam.x >> 3) <= 1)) {
				// sprite is due to be displayed soon, fetch its data
				fetch_is_sprite = 1;
			} else {
				fetch_is_sprite = 0;

				// decide whether we need to switch to window
				if (_io.reg[rLCDC] & LCDCF_WINON && pixel_y >= _io.reg[rWY] && (pixel_x >> 3) == (_io.reg[rWX] >> 3) - 2) {
					window = 1;
					bg_index_ctr = 0;
					// TODO: we don't do perfect horizontal positioning - we'll have to
					// refrain from pushing out the 0-7 last pixels from the bg fetches
				}
			}

			// cycle 0: generate tile map address and prepare reading index
			uint8_t xbase;
			uint8_t ybase;
			uint8_t index_ram_select_mask;
			if (fetch_is_sprite) {
				line_within_tile = current_y - spritegen[cur_sprite].oam.y + 16;
				if (spritegen[cur_sprite].oam.attr & 0x40) { // Y flip
					line_within_tile = get_sprite_height() - line_within_tile - 1;
				}
//				goto case1;
			} else {
				if (window) {
					xbase = bg_index_ctr;
					ybase = current_y - _io.reg[rWY];
					index_ram_select_mask = LCDCF_WIN9C00;
				} else {
					xbase = ((_io.reg[rSCX] >> 3) + bg_index_ctr) & 31;
					ybase = _io.reg[rSCY] + current_y;
					index_ram_select_mask = LCDCF_BG9C00;
				}
				uint8_t ybase_hi = ybase >> 3;
				line_within_tile = ybase & 7;
				uint16_t charaddr = 0x1800 | (!!(_io.reg[rLCDC] & index_ram_select_mask) << 10) | (ybase_hi << 5) | xbase;
				vram_set_address(charaddr);
			}
			break;
		}
		case 1: {
			// cycle 1: read index, generate tile data address and prepare reading tile data #0
//		case1:
			uint8_t index;
			if (fetch_is_sprite) {
				index = spritegen[cur_sprite].oam.tile;
			} else {
				index = vram_get_data();
			}
			if (fetch_is_sprite || (_io.reg[rLCDC] & LCDCF_BG8000)) {
				bgptr = index * 16;
			} else {
				bgptr = 0x1000 + (int8_t)index * 16;
			}
			bgptr += line_within_tile * 2;
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
			bool flip = fetch_is_sprite ? !(spritegen[cur_sprite].oam.attr & 0x20) : 0;
			int skip = bg_index_ctr ? 0 : _io.reg[rSCX] & 7; // bg only
			uint8_t palette = fetch_is_sprite ? _io.reg[spritegen[cur_sprite].oam.attr & 0x10 ? rOBP1 : rOBP0] : _io.reg[rBGP];
			for (int i = 7; i >= 0; i--) {
				int i2 = flip ? (7 - i) : i;
				bool b0 = (data0 >> i2) & 1;
				bool b1 = (data1 >> i2) & 1;
				uint8_t p2 = paletted(palette, b0 | (b1 << 1));
				if (fetch_is_sprite) {
					oam_pixel_set(i + (spritegen[cur_sprite].oam.x - pixel_x - 8), p2 ? p2 : 255);
				} else {
					if (skip) {
						skip--;
					} else {
						bg_pixel_push(p2);
					}
				}
			}
			if (fetch_is_sprite) {
				cur_sprite++;
			} else {
				bg_index_ctr++;
			}
			break;
		}
	}

	bg_t = (bg_t + 1 & 3);
}

void ppu::
pixel_step()
{
	if (bg_pixel_queue_next >= 8) {
		uint8_t p = bg_pixel_get();
		if (p != 0xff) {
			int line_full = !output_pixel(p);
			if (line_full) {
				// end this mode
				bg_pixel_queue_next = 0;
				new_line();
				mode = mode_hblank;
				vram_locked = 0;
				oamram_locked = 0;
			}
		}
	}
}

// PPU steps are executed the CPU clock rate, i.e. at ~4 MHz
void ppu::
step()
{
	if ((_io.reg[rLCDC] & LCDCF_ON)) {
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

	//
	// IRQ
	//

	// V-Blank interrupt
	if (current_y == 144 && current_x == 0) {
		_io.reg[rIF] |= 1;
	}
	// LY == LYC interrupt
	if (_io.reg[rSTAT] & 0x40 && _io.reg[rLYC] == current_y && current_x == 0) {
		_io.reg[rIF] |= 2;
	}
	if (mode != old_mode) {
		if (_io.reg[rSTAT] & 0x20 && mode == 2) {
			// Mode 2 interrupt
			_io.reg[rIF] |= 2;
		} else if (_io.reg[rSTAT] & 0x10 && mode == 1) {
			// Mode 1 interrupt
			_io.reg[rIF] |= 2;
		} else if (_io.reg[rSTAT] & 0x08 && mode == 0) {
			// Mode 0 interrupt
			_io.reg[rIF] |= 2;
		}
	}

	old_mode = mode;

	if (current_y <= PPU_LAST_VISIBLE_LINE) {
		if (mode == mode_oam) {
			// oam search runs at 4 MHz
			oam_step();
		}

		if (mode == mode_pixel) {
			// the background unit's speed is VRAM-bound, so it runs at 1/2 speed = 2 MHz
			if (!(current_x & 1)) {
				bg_step();
			}
		}
	}

	// shift pixels to the LCD @ 4 MHz
	pixel_step();

	if (++current_x == PPU_CLOCKS_PER_LINE) {
		current_x = 0;
//		printf("\n");
		if (++current_y == PPU_NUM_LINES) {
			current_y = 0;
			dirty = true;
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
