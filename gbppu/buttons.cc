//
//  buttons.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "buttons.h"
#include "io.h"

buttons::buttons(io &io)
	: _io     (io)
	, _buttons(0)
{
}

uint8_t buttons::
read()
{
	uint8_t d8 = _io.reg[rP1] | 0xcf;
	if (_io.reg[rP1] & 0x20) {
		d8 &= (_buttons ^ 0xff) | 0xf0;
	}
	if (_io.reg[rP1] & 0x10) {
		d8 &= ((_buttons ^ 0xff) >> 4) | 0xf0;
	}
	return d8;
}

void buttons::
write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;
}

void buttons::
set(uint8_t k)
{
    _buttons = k;
}
