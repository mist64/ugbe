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

class memory;
class io;

class ppu {
private:
	memory &_memory;
	io     &_io;

protected:
	friend class gb;
	ppu(memory &memory, io &io);

public:
	void step();

	uint8_t io_read(uint8_t a8);
	void io_write(uint8_t a8, uint8_t d8);

	uint8_t vram_read(uint16_t a16);
	void vram_write(uint16_t a16, uint8_t d8);
	uint8_t oamram_read(uint8_t a8);
	void oamram_write(uint8_t a8, uint8_t d8);

public:
	uint8_t picture[144][160];
    bool dirty;

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

	uint8_t line_within_tile;
	uint16_t bgptr;
	uint8_t data0;

	bool fetch_is_sprite;

#pragma pack(push, 1)
	typedef struct {
		uint8_t y;
		uint8_t x;
		uint8_t tile;
		uint8_t attr;
	} oamentry;
#pragma pack(pop)

	uint8_t active_sprite_index[10];

	void new_screen();
	int output_pixel(uint8_t p);
	void new_line();
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
