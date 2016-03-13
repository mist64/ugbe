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

extern ppu *ppu;

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


static void mem_write_internal(uint16_t a16, uint8_t d8);

static void
read_rom(const char *filename)
{
	uint8_t header[0x100];
	FILE *file = fopen(filename, "r");
	fseek(file, 0x100, SEEK_SET);
	fread(header, 0x100, 1, file);

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

	fseek(file, 0L, SEEK_SET);
	rom = (uint8_t *)calloc(romsize, 1);
	fread(rom, romsize, 1, file);
	fclose(file);

	rom_bank = 0;
}

void
mem_init()
{
	const char *bootrom_filename;
	const char *cartridge_filename;

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
	cartridge_filename = "/Users/mist/Documents/git/gb-timing/gb-timing.gb";
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
	cartridge_filename = "/Users/mist/tmp/gb/Tennis (W) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tesserae (U) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.0) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Tetris (W) (V1.1) [!].gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Trump Boy (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Volley Fire (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (J).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/World Bowling (U).gb";
//	cartridge_filename = "/Users/mist/tmp/gb/Yakyuuman (J).gb";

	cartridge_filename = "/Users/mist/Downloads/pocket/pocket.gb";
//	cartridge_filename = "/Users/mist/Downloads/oh/oh.gb";
//	cartridge_filename = "/Users/mist/Downloads/gejmboj/gejmboj.gb";
//	cartridge_filename = "/Users/mist/Downloads/SP-20Y/20y.gb";

//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/add_sp_e_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/mem_oam.gb"; // OK
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/reg_f.gb"; OK
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/bits/unused_hwio-GS.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/boot_hwio-G.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/boot_regs-dmg.gb"; // OK
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_cc_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_cc_timing2.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/call_timing2.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/di_timing-GS.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/div_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ei_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/hblank_ly_scx_timing-GS.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_1_2_timing-GS.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_0_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode0_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode0_timing_sprites.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_mode3_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/intr_2_oam_ok_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/gpu/vblank_stat_intr-GS.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime0_ei.gb"; // OK
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime0_nointr_timing.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime1_timing.gb"; // OK
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/halt_ime1_timing2-GS.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/if_ie_registers.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/intr_timing.gb"; // FAIL 
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/jp_cc_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/jp_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ld_hl_sp_e_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_restart.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_start.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/oam_dma_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/pop_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/push_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/rapid_di_ei.gb"; // FAILED
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ret_cc_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/ret_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/reti_intr_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/reti_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/rst_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/timer/div_write.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/acceptance/timer/rapid_toggle.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/emulator-only/mbc1_rom_4banks.gb"; // FAIL
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/external-bus/read_timing/read_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/external-bus/write_timing/write_timing.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/logic-analysis/ppu/simple_scx/simple_scx.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/madness/mgb_oam_dma_halt_sprites.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/manual-only/sprite_priority.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/bits/unused_hwio-C.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_hwio-C.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_hwio-S.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-A.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-cgb.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-mgb.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-sgb.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/boot_regs-sgb2.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/misc/gpu/vblank_stat_intr-C.gb";
//	cartridge_filename = "/Users/mist/tmp/mooneye-gb/tests/build/utils/dump_boot_hwio.gb";

	bootrom_filename = "/Users/mist/Documents/git/gbcpu/gbppu/DMG_ROM.bin";
#endif

	read_rom(cartridge_filename);

	bootrom = (uint8_t *)calloc(0x100, 1);
	FILE *file = fopen(bootrom_filename, "r");
	fread(bootrom, 0x100, 1, file);
	fclose(file);

	ram = (uint8_t *)calloc(0x2000, 1);
	hiram = (uint8_t *)calloc(0x7f, 1);

	extram = (uint8_t *)calloc(extramsize, 1);

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
			if ((rom_bank & 0x1f) == 0) {
				rom_bank++;
			}
			uint32_t address = a16 - 0x4000 + rom_bank * 0x4000;
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
		return ppu->ppu_vram_read(a16 - 0x8000);
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
		return ppu->ppu_oamram_read(a16 - 0xfe00);
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
		if (mbc == mbc1) {
			switch (a16 >> 13) {
				case 0:  /* 0x0000 - 0x1FFF: RAM enable */
					ram_enabled = (d8 & 0xf) == 0xa;
					break;
				case 1:  /* 0x2000 - 0x3FFF: ROM bank (lo) */
					rom_bank = (rom_bank & 0xe0 ) | (d8 & 0x1f);
					break;
				case 2: /* 0x4000 - 0x5FFF: RAM bank or ROM bank (hi) */
					if (!banking_mode) {
						rom_bank = (rom_bank & 0x1f) | ((d8 & 3) << 6);
					} else {
						ram_bank = d8 & 3;
					}
					break;
				case 3:
					banking_mode = d8 & 1;
					break;
			}
		} else {
			printf("warning: unsupported MBC write!\n");
		}
	} else if (a16 >= 0x8000 && a16 < 0xa000) {
		ppu->ppu_vram_write(a16 - 0x8000, d8);
	} else if (a16 >= 0xa000 && a16 < 0xc000) {
		if (a16 - 0xa000 < extramsize) {
			extram[a16 - 0xa000] = d8;
		} else {
			printf("warning: write to 0x%04x!\n", a16);
		}
	} else if (a16 >= 0xc000 && a16 < 0xe000) {
		ram[a16 - 0xc000] = d8;
	} else if (a16 >= 0xfe00 && a16 < 0xfea0) {
		ppu->ppu_oamram_write(a16 - 0xfe00, d8);
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
