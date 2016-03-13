//
//  ppu.h
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef ppu_h
#define ppu_h

#include <stdio.h>

class gb;
class memory;
class io;

class ppu {
	friend gb;
public:
	ppu();
	void ppu_step();

	uint8_t ppu_io_read(uint8_t a8);
	void ppu_io_write(uint8_t a8, uint8_t d8);

	uint8_t ppu_vram_read(uint16_t a16);
	void ppu_vram_write(uint16_t a16, uint8_t d8);
	uint8_t ppu_oamram_read(uint8_t a8);
	void ppu_oamram_write(uint8_t a8, uint8_t d8);

	uint8_t picture[144][160];
	int ppu_dirty;

protected:
	memory *memory;
	io *io;

private:
	uint8_t *vram;
	uint8_t *oamram;

	int current_x;
	int current_y;
	int oam_mode_counter;
	int bg_t; // internal BG fetch state (0-3)
	int bg_index_ctr; // offset of the current index within the line
	int window;

	uint8_t bg_pixel_queue[16];
	uint8_t bg_pixel_queue_next;
	uint8_t oam_pixel_queue[24];

	int screen_off;
	int vram_locked;
	int oamram_locked;

	int pixel_x, pixel_y;

	typedef enum {
		mode_hblank = 0,
		mode_vblank = 1,
		mode_oam    = 2,
		mode_pixel  = 3,
	} ppu_mode_t;
	ppu_mode_t mode;
	ppu_mode_t old_mode;

	uint16_t vram_address;
	uint8_t sprites_visible;
	uint8_t cur_sprite;

	uint8_t ybase;
	uint16_t bgptr;
	uint8_t data0;

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

	void new_screen();
	int ppu_output_pixel(uint8_t p);
	void ppu_new_line();
	uint8_t paletted(uint8_t pal, uint8_t p);
	void vram_set_address(uint16_t addr);
	uint8_t vram_get_data();
	uint8_t oam_get_pixel(uint8_t x);
	uint8_t get_sprite_height();
	void oam_step();
	void bg_reset();
	void bg_pixel_push(uint8_t p);
	uint8_t bg_pixel_get();
	void oam_pixel_set(int i, uint8_t p);
	uint8_t oam_pixel_get();
	void bg_step();
	void pixel_step();
};

#endif /* ppu_h */
