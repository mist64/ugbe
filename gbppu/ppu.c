//
//  ppu.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "ppu.h"
#include "ppu_private.h"
#include "memory.h"
#include "cpu.h"

uint8_t reg[256];

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

uint8_t pending_irqs;

int timer_counter;

static uint8_t keys;

void
io_set_keys(uint8_t k)
{
	keys = k;
}

void
timer_reset()
{
	timer_counter = 0;
}

void
timer_step()
{
	if (!(reg[rTAC] & 4)) {
		return;
	}

	int divider;
	switch (reg[rTAC] & 3) {
		default: // clang is stupid
		case 0:
			divider = 1024;
			break;
		case 1:
			divider = 16;
			break;
		case 2:
			divider = 64;
			break;
		case 3:
			divider = 256;
			break;
	}

//	printf("TIMER STEP %d %d %d\n", divider, timer_counter, reg[rTIMA]);

	if (++timer_counter == divider) {
		timer_counter = 0;
		if (++reg[rTIMA] == 0) {
			reg[rTIMA] = reg[rTMA];
//			printf("TIMER FIRED %s:%d\n", __FILE__, __LINE__);
			reg[rIF] |= 4;
		}
	}
}

static char *reg_name[] = {
	"P1", /* 0x00 */
	"SB", /* 0x01 */
	"SC", /* 0x02 */
	"0x03",
	"DIV", /* 0x04 */
	"TIMA", /* 0x05 */
	"TMA", /* 0x06 */
	"TAC", /* 0x07 */
	"0x08",
	"0x09",
	"0x0A",
	"0x0B",
	"0x0C",
	"0x0D",
	"0x0E",
	"IF", /* 0x0F */
	"NR10", /* 0x10 */
	"NR11", /* 0x11 */
	"NR12", /* 0x12 */
	"NR13", /* 0x13 */
	"NR14", /* 0x14 */
	"0x15",
	"NR21", /* 0x16 */
	"NR22", /* 0x17 */
	"NR23", /* 0x18 */
	"NR24", /* 0x19 */
	"NR30", /* 0x1A */
	"NR31", /* 0x1B */
	"NR32", /* 0x1C */
	"NR33", /* 0x1D */
	"NR34", /* 0x1E */
	"0x1F",
	"NR41", /* 0x20 */
	"NR42", /* 0x21 */
	"NR42_2", /* 0x22 */
	"NR43", /* 0x23 */
	"NR50", /* 0x24 */
	"NR51", /* 0x25 */
	"NR52", /* 0x26 */
	"0x27",
	"0x28",
	"0x29",
	"0x2A",
	"0x2B",
	"0x2C",
	"0x2D",
	"0x2E",
	"0x2F",
	"0x30",
	"0x31",
	"0x32",
	"0x33",
	"0x34",
	"0x35",
	"0x36",
	"0x37",
	"0x38",
	"0x39",
	"0x3A",
	"0x3B",
	"0x3C",
	"0x3D",
	"0x3E",
	"0x3F",
	"LCDC", /* 0x40 */
	"STAT", /* 0x41 */
	"SCY", /* 0x42 */
	"SCX", /* 0x43 */
	"LY", /* 0x44 */
	"LYC", /* 0x45 */
	"DMA", /* 0x46 */
	"BGP", /* 0x47 */
	"OBP0", /* 0x48 */
	"OBP1", /* 0x49 */
	"WY", /* 0x4A */
	"WX", /* 0x4B */
};

char *
name_for_io_reg(uint8_t a8)
{
	if (a8 == 0xff) {
		return "IE";
	} else if (a8 < sizeof(reg_name) / sizeof(*reg_name)) {
		return reg_name[a8];
	} else {
		return "???";
	}
}

uint8_t
io_read(uint8_t a8)
{
	switch (a8) {
		case rLY:
			return current_y;
		case rLCDC:
		case rSCX:
		case rSCY:
		case rBGP:
		case rOBP0:
		case rOBP1:
		case rWX:
		case rWY:
		case rSTAT:
		case rLYC:
		case rTAC:
		case 0xFF:
			// these behave like RAM
			return reg[a8];
		case rP1:{
			uint8_t d8 = reg[rP1] & 0xf0;
			if (reg[rP1] & 0x20) {
				d8 |= keys & 0xf;
			}
			if (reg[rP1] & 0x10) {
				d8 |= keys >> 4;
			}
			return d8;
		}
		default:
			printf("warning: I/O read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, reg[a8]);
			return reg[a8];
	}
}

void
io_write(uint8_t a8, uint8_t d8)
{
	extern uint16_t pc;

	switch (a8) {
		case 0x50:
			if (d8 & 1) {
				disable_bootrom();
			}
			break;
		case rLCDC:
		case rSCX:
		case rSCY:
		case rBGP:
		case rOBP0:
		case rOBP1:
		case rWX:
		case rWY:
		case rSTAT:
		case rLYC:
		case rP1:
		case 0xFF:
			reg[a8] = d8;
			break;
		case rDMA: {
			// TODO this should take a while and block access
			// to certain memory areas
			for (uint8_t i = 0; i < 160; i++) {
				mem_write(0xfe00 + i, mem_read((d8 << 8) + i));
			}
		}
		case rIF: {
			// set pending IRQs
			reg[a8] = d8;
			break;
		}
		case rTAC:
		case rTIMA:
			reg[a8] = d8;
			timer_reset();
			break;
		default:
			printf("warning: I/O write %s 0xff%02x <- 0x%02x (pc=%04x)\n", name_for_io_reg(a8), a8, d8, pc);
			reg[a8] = d8;
			break;
	}
}

uint8_t
io_get_pending_irqs()
{
	return reg[rIF];
}

void
io_clear_pending_irq(uint8_t irq)
{
	reg[rIF] &= ~(1 << irq);
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
	return vram_read(vram_address);
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

void
oam_step()
{
	extern uint8_t *oamram;
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
				if (!sprite_used[i] && oam[i].x && oam[i].y >= current_y && oam[i].y < current_y + sprite_height && oam[i].x < minx) {
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

//		if (sprites_visible) {
//			for (int j = 0; j < sprites_visible; j++) {
//				printf("line: %d; sprit e %d: x=%d, y=%d, index=%d\n", current_y, j, spritegen[j].oam.x, spritegen[j].oam.y, spritegen[j].oam.tile);
//			}
//		}
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
	timer_step();

//	if ((current_x & 3) == 0) {
//		printf("%c", mode + '0');
//	}

	void cpu_update_irq();

	uint8_t ie = io_read(rIE);
	if (cpu_ie()) {
		if (ie & 1 && current_y == 144 && current_x == 0) {
			reg[rIF] |= 1;
		} else {
			if (ie & 2) {
				if (io_read(rSTAT) & 0x40 && io_read(rLYC) == current_y && current_x == 0) {
					reg[rIF] |= 2;
				}
			}
		}

	}

	if (current_y <= PPU_LAST_VISIBLE_LINE) {

		if (mode == mode_oam) {
			oam_step();
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
//		printf("\n");
		if (++current_y == PPU_NUM_LINES) {
			current_y = 0;
			ppu_dirty = 1;
//			printf("\n");
		}
		if (current_y <= PPU_LAST_VISIBLE_LINE) {
			mode = mode_oam;
			oam_mode_counter = 0;
		} else {
			mode = mode_vblank;
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
