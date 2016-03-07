//
//  io.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdio.h>

#include "io.h"
#include "memory.h"
#include "buttons.h"
#include "serial.h"
#include "timer.h"
#include "sound.h"
#include "ppu.h"

uint8_t io[256];

static char *reg_name[] = {
	"P1", "SB", "SC", 0, "DIV", "TIMA", "TMA", "TAC", 0, 0, 0, 0, 0, 0, 0, "IF",
 "NR10", "NR11", "NR12", "NR13", "NR14", 0, "NR21", "NR22", "NR23", "NR24",
	"NR30", "NR31", "NR32", "NR33", "NR34", 0, "NR41", "NR42", "NR42_2", "NR43",
 "NR50", "NR51", "NR52", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, "LCDC", "STAT", "SCY", "SCX", "LY", "LYC", "DMA",
	"BGP", "OBP0", "OBP1", "WY", "WX"
};

char *
name_for_io_reg(uint8_t a8)
{
	if (a8 == 0xff) {
		return "IE";
	} else if (a8 < sizeof(reg_name) / sizeof(*reg_name)) {
		return reg_name[a8];
	} else {
		return 0;
	}
}

uint8_t
irq_read(uint8_t a8)
{
	// these behave like RAM
	return io[a8];
}

void
irq_write(uint8_t a8, uint8_t d8)
{
	// these behave like RAM
	io[a8] = d8;
}

uint8_t
io_get_pending_irqs()
{
	return io[rIF] & io[rIE];
}

void
io_clear_pending_irq(uint8_t irq)
{
	io[rIF] &= ~(1 << irq);
}


uint8_t
io_read(uint8_t a8)
{
	if (a8 == 0x00) {
		return buttons_read();
	} else if (a8 == 0x01 || a8 == 0x02) {
		return serial_read(a8);
	} else if (a8 >= 0x04 && a8 <= 0x07) {
		return timer_read(a8);
	} else if (a8 == 0x0F || a8 == 0xFF) {
		return irq_read(a8);
	} else if (a8 >= 0x10 && a8 <= 0x26) {
		return sound_read(a8);
	} else if (a8 >= 0x40 && a8 <= 0x4b) {
		return ppu_read(a8);
	} else if (a8 >= 0x80 && a8 <= 0xFE) {
		return io[a8]; // hi ram
	} else {
		// unassigned
		return 0xff;
	}
}

void
io_write(uint8_t a8, uint8_t d8)
{
	if (a8 == 0x00) {
		buttons_write(a8, d8);
	} else if (a8 == 0x01 || a8 == 0x02) {
		serial_write(a8, d8);
	} else if (a8 >= 0x04 && a8 <= 0x07) {
		timer_write(a8, d8);
	} else if (a8 == 0x0F || a8 == 0xFF) {
		irq_write(a8, d8);
	} else if (a8 >= 0x10 && a8 <= 0x26) {
		sound_write(a8, d8);
	} else if (a8 >= 0x40 && a8 <= 0x4b) {
		ppu_write(a8, d8);
	} else if (a8 == 0x50) {
		mem_io_write(a8, d8);
	} else if (a8 >= 0x80 && a8 <= 0xFE) {
		io[a8] = d8; // hi ram
	} else {
		// do nothing
	}
}

void
io_step()
{
	timer_step();
	ppu_step();
}

void
io_step_4()
{
	io_step();
	io_step();
	io_step();
	io_step();
}

