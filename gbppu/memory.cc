//
//  memory.c
//  gbppu
//
//  Created by Michael Steil on 2016-02-28
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdlib.h>
#include "memory.h"
#include "io.h"
#include "ppu.h"

void memory::
read_bootrom(const char *filename)
{
    bootrom = (uint8_t *)calloc(0x100, 1);
    FILE *file = fopen(filename, "r");
    if (file) {
        fread(bootrom, 0x100, 1, file);
        fclose(file);
    }
    
    bootrom_enabled = 1;
}

void memory::
read_rom(const char *filename)
{
	uint8_t header[0x100];
	FILE *file = fopen(filename, "r");
	if (file) {
		fseek(file, 0x100, SEEK_SET);
		fread(header, 0x100, 1, file);
	}

	mbc = mbc_none;
	has_ram = 0;
	has_battery = 0;
	has_timer = 0;

	switch (header[0x47]) {
		default:
		case 0x00: /* ROM ONLY */
			break;
		case 0x01: /* MBC1 */
			mbc = mbc1;
			break;
		case 0x02: /* MBC1+RAM */
			mbc = mbc1;
			has_ram = 1;
			break;
		case 0x03: /* MBC1+RAM+BATTERY */
			mbc = mbc1;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x05: /* MBC2 */
			mbc = mbc2;
			break;
		case 0x06: /* MBC2+BATTERY */
			mbc = mbc2;
			has_battery = 1;
			break;
		case 0x08: /* ROM+RAM */
			has_ram = 1;
			break;
		case 0x09: /* ROM+RAM+BATTERY */
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x0B: /* MMM01 */
			mbc = mmm01;
			break;
		case 0x0C: /* MMM01+RAM */
			mbc = mmm01;
			has_ram = 1;
			break;
		case 0x0D: /* MMM01+RAM+BATTERY */
			mbc = mmm01;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x0F: /* MBC3+TIMER+BATTERY */
			mbc = mbc3;
			has_timer = 1;
			has_battery = 1;
			break;
		case 0x10: /* MBC3+TIMER+RAM+BATTERY */
			mbc = mbc3;
			has_timer = 1;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x11: /* MBC3 */
			mbc = mbc3;
			break;
		case 0x12: /* MBC3+RAM */
			mbc = mbc3;
			has_ram = 1;
			break;
		case 0x13: /* MBC3+RAM+BATTERY */
			mbc = mbc3;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x15: /* MBC4 */
			mbc = mbc4;
			break;
		case 0x16: /* MBC4+RAM */
			mbc = mbc4;
			has_ram = 1;
			break;
		case 0x17: /* MBC4+RAM+BATTERY */
			mbc = mbc4;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x19: /* MBC5 */
			mbc = mbc5;
			break;
		case 0x1A: /* MBC5+RAM */
			mbc = mbc5;
			has_ram = 1;
			break;
		case 0x1B: /* MBC5+RAM+BATTERY */
			mbc = mbc5;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0x1C: /* MBC5+RUMBLE */
			mbc = mbc5;
			has_rumble = 1;
			break;
		case 0x1D: /* MBC5+RUMBLE+RAM */
			mbc = mbc5;
			has_rumble = 1;
			has_ram = 1;
			break;
		case 0x1E: /* MBC5+RUMBLE+RAM+BATTERY */
			mbc = mbc5;
			has_rumble = 1;
			has_ram = 1;
			has_battery = 1;
			break;
		case 0xFC: /* POCKET CAMERA */
			break;
		case 0xFD: /* BANDAI TAMA5 */
			break;
		case 0xFE: /* HuC3 */
			mbc = huc3;
			break;
		case 0xFF: /* HuC1+RAM+BATTERY */
			mbc = huc1;
			has_ram = 1;
			has_battery = 1;
			break;
	}

	switch (header[0x48]) {
		default:
		case 0x00: /*  32KByte (no ROM banking) */
			romsize = 32 * 1024;
			break;
		case 0x01: /*  64KByte (4 banks) */
			romsize = 4 * 16384;
			break;
		case 0x02: /* 128KByte (8 banks) */
			romsize = 8 * 16384;
			break;
		case 0x03: /* 256KByte (16 banks) */
			romsize = 16 * 16384;
			break;
		case 0x04: /* 512KByte (32 banks) */
			romsize = 32 * 16384;
			break;
		case 0x05: /*   1MByte (64 banks)  - only 63 banks used by MBC1 */
			romsize = 64 * 16384;
			break;
		case 0x06: /*   2MByte (128 banks) - only 125 banks used by MBC1 */
			romsize = 128 * 16384;
			break;
		case 0x07: /*   4MByte (256 banks) */
			romsize = 256 * 16384;
			break;
		case 0x52: /* 1.1MByte (72 banks) */
			romsize = 72 * 16384;
			break;
		case 0x53: /* 1.2MByte (80 banks) */
			romsize = 80 * 16384;
			break;
		case 0x54: /* 1.5MByte (96 banks) */
			romsize = 96 * 16384;
			break;
	}

	if (has_ram) {
		if (mbc == mbc2) {
			extramsize = 512; /* TODO nybbles! */
		} else {
			switch (header[0x49]) {
				default:
				case 0:
					extramsize = 0;
					break;
				case 1:
					extramsize = 0x800;
					break;
				case 2:
					extramsize = 0x2000;
					break;
				case 3:
					extramsize = 0x8000;
					break;
			}
		}
	}

    if (file) {
		fseek(file, 0L, SEEK_SET);
		rom = (uint8_t *)calloc(romsize, 1);
		fread(rom, romsize, 1, file);
		fclose(file);
    }

	rom_bank = 0;
    if (extramsize == 0 && extram) {
        free(extram);
        extram = 0;
    }
    if (extramsize > 0) {
        if (extram) {
            extram = (uint8_t *)realloc(extram, extramsize);
        } else {
            extram = (uint8_t *)calloc(extramsize, 1);
        }
    }
}

memory::memory(ppu &ppu, io &io, const char *bootrom_filename, const char *cartridge_filename)
	: _ppu(ppu)
	, _io (io)
{
    extram = 0;
    read_rom(cartridge_filename);
    read_bootrom(bootrom_filename);

    ram = (uint8_t *)calloc(0x2000, 1);
    hiram = (uint8_t *)calloc(0x7f, 1);
    

#if 0
	file = fopen("/Users/mist/Documents/git/gbcpu/gbppu/ram.bin", "r");
	uint8_t *data = malloc(65536);
	fread(data, 65536, 1, file);
	fclose(file);

	for (int addr = 0; addr < 65536; addr++) {
		write_internal(addr, data[addr]);
	}
#endif
}

uint8_t memory::
read(uint16_t a16)
{
	_io.io_step_4();

	if (a16 < 0x4000) {
		if (bootrom_enabled && a16 < 0x100) {
			return bootrom[a16];
		} else {
			return rom[a16];
		}
	} else if (a16 >= 0x4000 && a16 < 0xa000) {
		if (mbc == mbc_none) {
			return rom[a16];
		} else if (mbc == mbc1) {
			uint8_t bank = rom_bank;
			if (banking_mode) {
				bank &= 0x1f;
			}
			if ((bank & 0x1f) == 0) {
				bank++;
			}
			uint32_t address = a16 - 0x4000 + bank * 0x4000;
			if (address > romsize) {
				return 0xff;
			} else {
				return rom[address];
			}
		} else {
			printf("warning: unsupported MBC read!\n");
			return rom[a16];
		}
	} else if (a16 >= 0x8000 && a16 < 0xa000) {
		return _ppu.vram_read(a16 - 0x8000);
	} else if (a16 >= 0xa000 && a16 < 0xc000) {
		// TODO: RAM banking
		if (a16 - 0xa000 < extramsize) {
			return extram[a16 - 0xa000];
		} else {
			printf("warning: read from 0x%04x!\n", a16);
			return 0;
		}
	} else if (a16 >= 0xc000 && a16 < 0xe000) {
		return ram[a16 - 0xc000];
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		return _ppu.oamram_read(a16 - 0xfe00);
	} else if (a16 >= 0xfea0 && a16 < 0xff00) {
		// unassigned
		return 0xff;
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		return _io.io_read(a16 & 0xff);
	} else if (a16 >= 0xff80) {
		return hiram[a16 - 0xff80];
	} else {
		printf("warning: read from 0x%04x!\n", a16);
		return 0xff;
	}
}

void memory::
write_internal(uint16_t a16, uint8_t d8)
{
	if (a16 < 0x8000) {
		if (mbc == mbc1) {
			switch (a16 >> 13) {
				case 0:  /* 0x0000 - 0x1FFF: RAM enable */
					ram_enabled = (d8 & 0xf) == 0xa;
					break;
				case 1:  /* 0x2000 - 0x3FFF: ROM bank (lo) */
					rom_bank = (rom_bank & 0xe0) | (d8 & 0x1f);
					break;
				case 2: /* 0x4000 - 0x5FFF: RAM bank or ROM bank (hi) */
					if (!banking_mode) {
						rom_bank = (rom_bank & 0x1f) | ((d8 & 3) << 6);
					} else {
						ram_bank = d8 & 3;
					}
					break;
				case 3: /* 0x6000 - 0x7FFF: ROM/RAM Mode Select */
					banking_mode = d8 & 1;
					break;
			}
		} else {
			printf("warning: unsupported MBC write!\n");
		}
	} else if (a16 >= 0x8000 && a16 < 0xa000) {
		_ppu.vram_write(a16 - 0x8000, d8);
	} else if (a16 >= 0xa000 && a16 < 0xc000) {
		if (a16 - 0xa000 < extramsize) {
			extram[a16 - 0xa000] = d8;
		} else {
			printf("warning: write to 0x%04x!\n", a16);
		}
	} else if (a16 >= 0xc000 && a16 < 0xe000) {
		ram[a16 - 0xc000] = d8;
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		_ppu.oamram_write(a16 - 0xfe00, d8);
	} else if (a16 >= 0xfea0 && a16 < 0xff00) {
		// unassigned
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		_io.io_write(a16 & 0xff, d8);
	} else if (a16 >= 0xff80) {
		hiram[a16 - 0xff80] = d8;
	} else {
		printf("warning: write to 0x%04x!\n", a16);
	}
}

void memory::
write(uint16_t a16, uint8_t d8)
{
	_io.io_step_4();
	write_internal(a16, d8);
}

void memory::
io_write(uint8_t a8, uint8_t d8)
{
	if (d8 & 1) {
		bootrom_enabled = 0;
	}
}

bool memory::
is_bootrom_enabled()
{
	return bootrom_enabled;
}
