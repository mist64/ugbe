//
//  sound.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "sound.h"
#include "io.h"

sound::sound(io &io)
	: _io(io)
{
	c1_freq = 0;
	c1_freq_counter = 0;
}

void sound::
c1_restart()
{
	switch (_io.reg[rNR11] >> 6) {
		case 0:
			c1_trigger = c1_freq >> 3;
			c1_invert = false;
			break;
		case 1:
			c1_trigger = c1_freq >> 2;
			c1_invert = false;
			break;
		case 2:
			c1_trigger = c1_freq >> 1;
			c1_invert = false;
			break;
		case 3:
			c1_trigger = c1_freq >> 2;
			c1_invert = true;
			break;
	}
	c1_freq_counter = c1_freq;
	c1_value = c1_invert;
}

uint8_t sound::
read(uint8_t a8)
{
	printf("warning: sound read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, _io.reg[a8]);
	return 0xff;
}

void sound::
write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;
	printf("warning: sound I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);

	if (a8 == rNR14) {
		if (d8 & 0x80) {
			c1_on = true;
			c1_freq = (((_io.reg[rNR13] & 7) << 8) | _io.reg[rNR14]) ^ 2047;
			c1_restart();
		}
	}
}

// runs at 131072 Hz
void sound::
step()
{
	clock_divider = (clock_divider + 1) & 7;
	if (clock_divider) {
		return;
	}

	if (c1_on) {
		c1_freq_counter--;
		if (c1_freq_counter == c1_trigger) {
			c1_value = !c1_invert;
		} else if (c1_freq_counter == 0) {
			c1_restart();
		}
		printf("%d", c1_value ? 1 : 0);
	}
}
