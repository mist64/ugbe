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

uint8_t *bootrom;
uint8_t *rom;
uint8_t *ram;
uint8_t *extram;
uint8_t *hiram;

uint16_t extramsize;
int bootrom_enabled;

static void mem_write_internal(uint16_t a16, uint8_t d8);

void
mem_init()
{
	char *bootrom_filename;
	char *cartridge_filename;

#if BUILD_USER_Lisa
	bootrom_filename = "/Users/lisa/Projects/game_boy/gbcpu/gbppu/DMG_ROM.bin";

// CPU Tests
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/cpu_instrs.gb";

/*0x27*/	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/01-special.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/02-interrupts.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/03-op sp,hl.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/04-op r,imm.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/05-op rp.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/06-ld r,r.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/07-jr,jp,call,ret,rst.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/08-misc instrs.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/09-op r,r.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/10-bit ops.gb";
///*0x27*/	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/cpu_test/individual/11-op a,(hl).gb";

// demos
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/demos/3dclown/CLOWN1.GB";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/demos/pocket/pocket.gb";

// games
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/games/Tennis (W) [!].gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/gbcpu/gbppu/tetris.gb";
//	cartridge_filename = "/Users/lisa/Projects/game_boy/roms/games/Pokemon - Blaue Edition (G) [S][!].gb";

#else
//	cartridge_filename = "/Users/mist/Documents/git/gbcpu/gbppu/tetris.gb";
//	cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-scy.gb";
//	cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-timing.gb";
//	cartridge_filename = "/Users/mist/Documents/git/gb-platformer/gb-platformer.gb";

//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/01-special.gb";            // x Failed #6 - DAA
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/02-interrupts.gb";         // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/03-op sp,hl.gb";           // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/04-op r,imm.gb";           // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/05-op rp.gb";              // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/06-ld r,r.gb";             // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"; // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/08-misc instrs.gb";        // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/09-op r,r.gb";             // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/10-bit ops.gb";            // Passed
//	cartridge_filename = "/Users/mist/Downloads/cpu_instrs/individual/11-op a,(hl).gb";          // x Failed - needs DAA

//	cartridge_filename = "/Users/mist/tmp/gb/1993 Collection 128-in-1 (Unl) [b1].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Alleyway (W) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Amida (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Artic Zone (Sachen 4-in-1 Vol. 5) (Unl) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Asteroids (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/BattleCity (J) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Bomb Disposer (Sachen 4-in-1 Vol. 6) (Unl) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Bomb Jack (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Boxxle (U) (V1.1) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Boxxle 2 (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Brainbender (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Bubble Ghost (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Bubble Ghost (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Castelian (E) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Castelian (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Catrap (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Centipede (E) [M][!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Challenger GB BIOS [C][!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Chiki Chiki Tengoku (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Cool Ball (E) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Crystal Quest (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Daedalean Opus (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Dan Laser (Sachen) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Dr. Mario (W) (V1.0) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Dr. Mario (W) (V1.1).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Dragon Slayer I (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Dropzone (U) [M].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Explosive Brick '94 (Sachen) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Flappy Special (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Flipull (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Flipull (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Game of Harmony, The (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Heiankyo Alien (J) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Heiankyo Alien (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Hong Kong (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Hyper Lode Runner (JU) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Ishidou (J) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Klax (J) [M].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Koi Wa Kakehiki (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Korodice (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Kwirk (UE) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Kyorochan Land (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Loopz (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Magic Maze (Sachen) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Master Karateka (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/MegaMemory V1.0 by Interact (E).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Migrain (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Minesweeper (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Missile Command (U) [M][!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Mogura De Pon! (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (E) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Motocross Maniacs (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/NFL Football (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Othello (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Othello (UE) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Palamedes (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Palamedes (UE) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Penguin Land (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Pipe Dream (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Pipe Dream (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Pitman (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Pop Up (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Puzzle Boy (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Puzzle Road (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Q Billion (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Q Billion (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Renju Club (J) [S].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Serpent (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Serpent (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Shanghai (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Shanghai (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Shisenshou - Match-Mania (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Soukoban (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Soukoban 2 (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Space Invaders (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Spot (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Spot (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Street Rider (Sachen 4-in-1 Vol. 1) (Unl) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Supreme 105 in 1 (Menu) [p1][b1].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tasmania Story (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tasmania Story (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tennis (W) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tesserae (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.0) [!].gb";
	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.1) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Trump Boy (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Volley Fire (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Yakyuuman (J).gb";

	

	bootrom_filename = "/Users/mist/Documents/git/gbcpu/gbppu/DMG_ROM.bin";
#endif
    mem_init_filenames(bootrom_filename, cartridge_filename);
}

void
mem_init_filenames(char *bootrom_filename, char *cartridge_filename)
{
	FILE *file = fopen(cartridge_filename, "r");

	fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	if (size < 0x8000) {
		size = 0x8000;
	}
	fseek(file, 0L, SEEK_SET);
	rom = calloc(size, 1);
	fread(rom, size, 1, file);
	fclose(file);

	bootrom = calloc(0x100, 1);
	file = fopen(bootrom_filename, "r");
	fread(bootrom, 0x100, 1, file);
	fclose(file);

	ram = calloc(0x2000, 1);
	hiram = calloc(0x7f, 1);

	switch (rom[0x149]) {
		default:
		case 0:
			// TODO: MBC2
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
	extram = calloc(extramsize, 1);

	bootrom_enabled = 1;

#if 0
	file = fopen("/Users/mist/Documents/git/gbcpu/gbppu/ram.bin", "r");
	uint8_t *data = malloc(65536);
	fread(data, 65536, 1, file);
	fclose(file);

	for (int addr = 0; addr < 65536; addr++) {
		mem_write_internal(addr, data[addr]);
	}
#endif
}

uint8_t
mem_read(uint16_t a16)
{
	io_step_4();

	if (a16 < 0x8000) {
		if (bootrom_enabled && a16 < 0x100) {
			return bootrom[a16];
		} else {
			return rom[a16];
		}
	} else if (a16 >= 0x8000 && a16 < 0xa000) {
		return ppu_vram_read(a16 - 0x8000);
	} else if (a16 >= 0xa000 && a16 < 0xc000) {
		if (a16 - 0xa000 < extramsize) {
			return extram[a16 - 0xa000];
		} else {
			printf("warning: read from 0x%04x!\n", a16);
			return 0;
		}
	} else if (a16 >= 0xc000 && a16 < 0xe000) {
		return ram[a16 - 0xc000];
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		return ppu_oamram_read(a16 - 0xfe00);
	} else if (a16 >= 0xfea0 && a16 < 0xff00) {
		// unassigned
		return 0xff;
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		return io_read(a16 & 0xff);
	} else if (a16 >= 0xff80) {
		return hiram[a16 - 0xff80];
	} else {
		printf("warning: read from 0x%04x!\n", a16);
		return 0xff;
	}
}

static void
mem_write_internal(uint16_t a16, uint8_t d8)
{
	if (a16 < 0x8000) {
		// TODO: MBC
	} else if (a16 >= 0x8000 && a16 < 0xa000) {
		ppu_vram_write(a16 - 0x8000, d8);
	} else if (a16 >= 0xa000 && a16 < 0xc000) {
		if (a16 - 0xa000 < extramsize) {
			extram[a16 - 0xa000] = d8;
		} else {
			printf("warning: write to 0x%04x!\n", a16);
		}
	} else if (a16 >= 0xc000 && a16 < 0xe000) {
		ram[a16 - 0xc000] = d8;
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		ppu_oamram_write(a16 - 0xfe00, d8);
	} else if (a16 >= 0xfea0 && a16 < 0xff00) {
		// unassigned
	} else if ((a16 >= 0xff00 && a16 < 0xff80) || a16 == 0xffff) {
		io_write(a16 & 0xff, d8);
	} else if (a16 >= 0xff80) {
		hiram[a16 - 0xff80] = d8;
	} else {
		printf("warning: write to 0x%04x!\n", a16);
	}
}

void
mem_write(uint16_t a16, uint8_t d8)
{
	io_step_4();
	mem_write_internal(a16, d8);
}

void
mem_io_write(uint8_t a8, uint8_t d8)
{
	if (d8 & 1) {
		bootrom_enabled = 0;
	}
}

int
mem_is_bootrom_enabled()
{
	return bootrom_enabled;
}
