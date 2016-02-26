//
//  ppu.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "ppu.h"

#define PPU_NUM_LINES 154
#define PPU_CLOCKS_PER_LINE (114*2)
#define PPU_LAST_VISIBLE_LINE 143

#define rP1 0x00
#define rLCDC 0x40
#define rSTAT 0x41
#define rSCY  0x42
#define rSCX  0x43
#define rLY 0x44
#define rLYC  0x45
#define rDMA  0x46
#define rBGP  0x47
#define rOBP0 0x48
#define rOBP1 0x49
#define rSB 0x01
#define rSC 0x02
#define rDIV 0x04
#define rTIMA 0x05
#define rTMA 0x06
#define rTAC 0x07
#define rIF 0x0F
#define rIE 0xFF
#define rWY 0x4A
#define rWX 0x4B
#define rNR50 0x24
#define rNR51 0x25
#define rNR52 0x26
#define rNR10 0x10
#define rNR11 0x11
#define rNR12 0x12
#define rNR13 0x13
#define rNR14 0x14
#define rNR21 0x16
#define rNR22 0x17
#define rNR23 0x18
#define rNR24 0x19
#define rNR30 0x1A
#define rNR31 0x1B
#define rNR32 0x1C
#define rNR33 0x1D
#define rNR34 0x1E
#define rNR41 0x20
#define rNR42 0x21
#define rNR42_2 0x22
#define rNR43 0x23


#define LCDCF_ON      (1 << 7) /* LCD Control Operation */
#define LCDCF_WIN9C00 (1 << 6) /* Window Tile Map Display Select */
#define LCDCF_WINON   (1 << 5) /* Window Display */
#define LCDCF_BG8000  (1 << 4) /* BG & Window Tile Data Select */
#define LCDCF_BG9C00  (1 << 3) /* BG Tile Map Display Select */
#define LCDCF_OBJ16   (1 << 2) /* OBJ Construction */
#define LCDCF_OBJON   (1 << 1) /* OBJ Display */
#define LCDCF_BGON    (1 << 0) /* BG Display */

extern uint8_t RAM[];
uint8_t *reg = RAM + 0xFF00;

int current_x;
int current_y;
int mode;
int oam_mode_counter;
int pixel_transfer_mode_counter;

int pixel_x, pixel_y;
uint8_t picture[160][144];

int ppu_dirty;

uint8_t
io_read(uint16_t a16)
{
	if (a16 == 0xff44) {
		return current_y;
	} else {
		//		printf("%s:%d 0x%02x\n", __FILE__, __LINE__, a16);
		//		assert(false);
		return RAM[a16];
	}
}

void
ppu_init()
{
	mode = 2; // OAM
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

// PPU steps are executed at half the CPU clock rate, i.e. at ~2 MHz
void
ppu_step()
{
	static uint8_t ybase;
	static uint16_t bgptr;
	static uint8_t data0;

	if (current_y <= PPU_LAST_VISIBLE_LINE) {
		switch (mode) {
			case 0: // HBLANK
				break;
			case 2: { // OAM
				if (++oam_mode_counter == 40) {
					mode = 3;
					pixel_transfer_mode_counter = 0;
				}
				break;
			case 3: // pixel transfer
				switch (pixel_transfer_mode_counter & 3) {
					case 0: {
						// cycle 0: generate tile map address and prepare reading index
						uint8_t xbase = ((reg[rSCX] >> 3) + pixel_transfer_mode_counter / 4) & 31;
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
						int start = (pixel_transfer_mode_counter >> 2) ? 7 : (7 - (reg[rSCX] & 7));

						for (int i = start; i >= 0; i--) {
							int b0 = (data0 >> i) & 1;
							int b1 = (data1 >> i) & 1;
							//							printf("%c", printable(paletted(reg[rBGP], b0 | b1 << 1)));
							if (!ppu_output_pixel(paletted(reg[rBGP], b0 | b1 << 1))) {
								// line is full, end this
								ppu_new_line();
								mode = 0;
								break;
							}
						}
						break;
					}
				}
				pixel_transfer_mode_counter++;
			}
		}
	}


	if (current_x == PPU_CLOCKS_PER_LINE - 1) {
		//		printf("\n");
		if (current_y == PPU_LAST_VISIBLE_LINE + 1) {
			//			printf("\n");
		}
	}

	if (++current_x == PPU_CLOCKS_PER_LINE) {
		current_x = 0;
		mode = 2;
		oam_mode_counter = 0;
		if (++current_y == PPU_NUM_LINES) {
			current_y = 0;
			ppu_dirty = 1;
		}
	}
}
