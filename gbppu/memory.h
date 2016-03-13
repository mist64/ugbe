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
#include <stdint.h>

class io;
class ppu;

class memory {
private:
	ppu &_ppu;
	io  &_io;

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

	void write_internal(uint16_t a16, uint8_t d8);

protected:
	friend class gb;
	memory(ppu &ppu, io &io, const char *bootrom_filename, const char *cartridge_filename);

public:
	void init();

    void read_bootrom(const char *filename);
    void read_rom(const char *filename);
    
    uint8_t read(uint16_t a16);
	void write(uint16_t a16, uint8_t d8);

	void io_write(uint8_t a8, uint8_t d8);

	int is_bootrom_enabled();
};

#endif /* memory_h */
