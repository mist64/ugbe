//
//  io.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "buttons.h"
#include "serial.h"
#include "timer.h"
#include "sound.h"
#include "ppu.h"

#include "io.h"

static const char *reg_name[] = {
	"P1", "SB", "SC", 0, "DIV", "TIMA", "TMA", "TAC", 0, 0, 0, 0, 0, 0, 0, "IF",
 "NR10", "NR11", "NR12", "NR13", "NR14", 0, "NR21", "NR22", "NR23", "NR24",
	"NR30", "NR31", "NR32", "NR33", "NR34", 0, "NR41", "NR42", "NR42_2", "NR43",
 "NR50", "NR51", "NR52", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, "LCDC", "STAT", "SCY", "SCX", "LY", "LYC", "DMA",
	"BGP", "OBP0", "OBP1", "WY", "WX"
};

const char *
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

io::io(ppu &ppu, memory &memory, timer &timer, serial &serial, buttons &buttons, sound &sound)
	: _ppu    (ppu)
	, _memory (memory)
	, _timer  (timer)
	, _serial (serial)
	, _buttons(buttons)
	, _sound  (sound)
{
	memset(reg, 0, sizeof(reg));
}

uint8_t io::
irq_read(uint8_t a8)
{
	// these behave like RAM
	return reg[a8];
}

void io::
irq_write(uint8_t a8, uint8_t d8)
{
	// these behave like RAM
	reg[a8] = d8;
}

uint8_t io::
irq_get_pending()
{
	return reg[rIF] & reg[rIE];
}

void io::
irq_set_pending(uint8_t irq)
{
	reg[rIF] |= (1 << irq);
}

void io::
irq_clear_pending(uint8_t irq)
{
	reg[rIF] &= ~(1 << irq);
}


uint8_t io::
io_read(uint8_t a8)
{
	if (a8 == 0x00) {
		return _buttons.read();
	} else if (a8 == 0x01 || a8 == 0x02) {
		return _serial.read(a8);
	} else if (a8 >= 0x04 && a8 <= 0x07) {
		return _timer.read(a8);
	} else if (a8 == 0x0F || a8 == 0xFF) {
		return irq_read(a8);
	} else if (a8 >= 0x10 && a8 <= 0x26) {
		return _sound.read(a8);
	} else if (a8 >= 0x40 && a8 <= 0x4b) {
		return _ppu.io_read(a8);
	} else {
		// unassigned
		return 0xff;
	}
}

void io::
io_write(uint8_t a8, uint8_t d8)
{
	if (a8 == 0x00) {
		_buttons.write(a8, d8);
	} else if (a8 == 0x01 || a8 == 0x02) {
		_serial.write(a8, d8);
	} else if (a8 >= 0x04 && a8 <= 0x07) {
		_timer.write(a8, d8);
	} else if (a8 == 0x0F || a8 == 0xFF) {
		irq_write(a8, d8);
	} else if (a8 >= 0x10 && a8 <= 0x26) {
		_sound.write(a8, d8);
	} else if (a8 >= 0x40 && a8 <= 0x4b) {
		_ppu.io_write(a8, d8);
	} else if (a8 == 0x50) {
		_memory.io_write(a8, d8);
	} else {
		// do nothing
	}
}

void io::
io_step()
{
	_timer.step();
	_ppu.step();
}

void io::
io_step_4()
{
	io_step();
	io_step();
	io_step();
	io_step();
}
