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

enum {
	source_bg,
	source_obj0,
	source_obj1,
	source_invalid,
};

typedef struct {
	unsigned char value : 2;
	unsigned char source : 2;
} pixel_t;

class ppu {
private:
	memory &_memory;
	io     &_io;

protected:
	friend class gb;
	ppu(memory &memory, io &io);
    uint8_t *vram;

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
	uint8_t *oamram;

	bool clock_even;
	int clock;
	int oam_counter;
	int oam_out_counter;
	int oam_t;
	bool oam_candidate;
	int bg_t; // internal BG fetch state (0-3)
	int bg_index_ctr; // offset of the current index within the line
	int window;

	pixel_t bg_pixel_queue[16];

	bool screen_off;
	bool vram_locked;
	bool oamram_locked;

	int pixel_x;
	int line;
	uint8_t skip;
	uint8_t *ppicture;

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

	uint8_t line_within_tile;
	uint16_t bgptr;
	uint8_t data0;

	bool fetch_is_sprite;
	uint8_t bg_count;
	int sprite_index;

#pragma pack(push, 1)
	typedef struct {
		uint8_t y;
		uint8_t x;
		uint8_t tile;
		uint8_t attr;
	} oamentry;
#pragma pack(pop)

	int8_t active_sprite_index[10];
	oamentry *cur_oam;

	char debug_string_pixel[1024];
	char debug_string_fetch[1024];
	void debug_init();
	void debug_pixel(char);
	void debug_fetch(char);
	void debug_flush();

	void screen_reset();
	int output_pixel(uint8_t p);
	void new_line();
	void vram_set_address(uint16_t addr);
	uint8_t vram_get_data();
	uint8_t oam_get_pixel(uint8_t x);
	uint8_t get_sprite_height();
	void oam_reset();
	void oam_step();

	void hblank_reset();
	void hblank_step();
	void vblank_reset();
	void vblank_step();

	void irq_step();

	void line_reset();

	int sprite_starts_here();
	void pixel_reset();
	void pixel_step();
	void fetch_step();
};

#endif /* ppu_h */
