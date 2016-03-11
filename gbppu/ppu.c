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
#include "ppu_private.h"
#include "memory.h"
#include "io.h"

uint8_t *vram;
uint8_t *oamram;

int current_x;
int current_y;
int oam_mode_counter;
int bg_t; // internal BG fetch state (0-3)
int bg_index_ctr; // offset of the current index within the line

static uint8_t bg_pixel_queue[16];
static uint8_t bg_pixel_queue_next;
static uint8_t oam_pixel_queue[24];

int screen_off;
int vram_locked;
int oamram_locked;

int pixel_x, pixel_y;
uint8_t picture[144][160];

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

	bg_pixel_queue_next = 0;
	memset(oam_pixel_queue, 0xff, sizeof(oam_pixel_queue));
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

static uint8_t sprites_visible;
static uint8_t cur_sprite;

static uint8_t oam_pixel_get();

uint8_t
oam_get_pixel(uint8_t x)
{
	return oam_pixel_get();
}

static uint8_t
get_sprite_height()
{
	return io_read(rLCDC) & LCDCF_OBJ16 ? 16 : 8;
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
				sprites_visible++;
			}
		} while (sprites_visible < 10 && oam_index >= 0);
//		printf("\n");

		cur_sprite = 0;
	}
}

void
bg_reset()
{
	bg_index_ctr = 0;
	bg_t = 0;
}

//static int fetch_is_sprite;

static void
bg_pixel_push(uint8_t p)
{
	bg_pixel_queue[bg_pixel_queue_next++] = p;
	assert(bg_pixel_queue_next <= sizeof(bg_pixel_queue));
}

static uint8_t
bg_pixel_get()
{
	if (!bg_pixel_queue_next) {
		return 0xff;
	}
	uint8_t p = bg_pixel_queue[0];
	memmove(bg_pixel_queue, bg_pixel_queue + 1, --bg_pixel_queue_next);
	return p;
}

static void
oam_pixel_set(int i, uint8_t p)
{
	assert(i < sizeof(oam_pixel_queue));
	if (oam_pixel_queue[i] == 0xff) {
		oam_pixel_queue[i] = p;
	}
}

static uint8_t
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


static void
bg_step()
{
	static uint8_t ybase;
	static uint16_t bgptr;
	static uint8_t data0;

	// background @ 2 MHz
	switch (bg_t) {
		case 0: {
			// decide whether we should do a sprite tile fetch here
			while (cur_sprite != sprites_visible &&
				   (((spritegen[cur_sprite].oam.x >> 3) - 2) == (pixel_x >> 3) || (spritegen[cur_sprite].oam.x >> 3) <= 1)) {
				// sprite is due to be displayed soon, fetch its data
				uint8_t i = ((current_y - spritegen[cur_sprite].oam.y) & (get_sprite_height() - 1));
				if (spritegen[cur_sprite].oam.attr & 0x40) { // Y flip
					i = 7 - i;
				}
				uint16_t address = 16 * spritegen[cur_sprite].oam.tile + i * 2;

				for (int i = 0; i < 8; i++) {
					int i2 = i;
					if (!(spritegen[cur_sprite].oam.attr & 0x20)) { // X flip
						i2 = 7 - i2;
					}

					int b0 = (vram[address] >> i2) & 1;
					int b1 = (vram[address + 1] >> i2) & 1;
					uint8_t p2 = b0 | (b1 << 1);
					oam_pixel_set(i + (spritegen[cur_sprite].oam.x - pixel_x - 8), p2 ? paletted(io[spritegen[cur_sprite].oam.attr & 0x10 ? rOBP1 : rOBP0], p2) : 255);
				}

				spritegen[cur_sprite].data0 = vram[address];
				spritegen[cur_sprite].data1 = vram[address + 1];
				cur_sprite++;

//				fetch_is_sprite = 1;
//			} else {
//				fetch_is_sprite = 0;
			}

			// cycle 0: generate tile map address and prepare reading index
			uint8_t xbase = ((io[rSCX] >> 3) + bg_index_ctr) & 31;
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
			int skip = bg_index_ctr ? 0 : io[rSCX] & 7;
			for (int i = 7; i >= 0; i--) {
				int b0 = (data0 >> i) & 1;
				int b1 = (data1 >> i) & 1;
				if (skip) {
					skip--;
				} else {
					bg_pixel_push(paletted(io[rBGP], b0 | b1 << 1));
				}
			}

			bg_index_ctr++;
			break;
		}
	}

	bg_t = (bg_t + 1 & 3);
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
				vram_locked = 1;
				oamram_locked = 1;
				bg_reset();
			}
		}

		if (mode == mode_pixel) {
			// the background unit's speed is vram-bound, so it runs at 1/2 speed = 2 MHz
			if (!(current_x & 1)) {
				bg_step();
			}
		}
	}

	// shift pixels to the LCD @ 4 MHz
	if (bg_pixel_queue_next >= 8) {
		uint8_t p = bg_pixel_get();
		if (p != 0xff) {
			int line_full = !ppu_output_pixel(p);
			if (line_full) {
				// end this mode
				bg_pixel_queue_next = 0;
				ppu_new_line();
				mode = mode_hblank;
				vram_locked = 0;
				oamram_locked = 0;
			}
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
