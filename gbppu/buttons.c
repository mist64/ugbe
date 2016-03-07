//
//  buttons.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "buttons.h"
#include "io.h"

static uint8_t buttons;

uint8_t
buttons_read()
{
	uint8_t d8 = io[rP1] & 0xf0;
	if (io[rP1] & 0x20) {
		d8 |= (buttons & 0xf) ^ 0xf;
	}
	if (io[rP1] & 0x10) {
		d8 |= (buttons >> 4) ^ 0xf;
	}
	return d8;
}

void
buttons_write(uint8_t a8, uint8_t d8)
{
	io[a8] = d8;
}

void
buttons_set(uint8_t k)
{
	buttons = k;
}

