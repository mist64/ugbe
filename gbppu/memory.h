//
//  memory.h
//  gbppu
//
//  Created by Michael Steil on 2016-02-28
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef memory_h
#define memory_h

#include <stdio.h>

class memory {
private:
	uint8_t *bootrom;
	uint8_t *rom;
	uint8_t *ram;
	uint8_t *extram;
	uint8_t *hiram;

	enum {
		mbc_none,
		mbc1,
		mbc2,
		mbc3,
		mbc4,
		mbc5,
		mmm01,
		huc1,
		huc3,
	} mbc;
	int has_ram;
	int has_battery;
	int has_timer;
	int has_rumble;
	uint32_t romsize;
	uint16_t extramsize;
	int ram_enabled;
	uint8_t rom_bank;
	uint8_t ram_bank;
	int banking_mode;
	int bootrom_enabled;

	void read_rom(const char *filename);
	void mem_write_internal(uint16_t a16, uint8_t d8);

public:
	memory();
	void mem_init();
	uint8_t mem_read(uint16_t a16);
	void mem_write(uint16_t a16, uint8_t d8);

	void mem_io_write(uint8_t a8, uint8_t d8);

	int mem_is_bootrom_enabled();
};

#endif /* memory_h */
