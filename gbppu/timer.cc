//
//  timer.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "timer.h"
#include "io.h"

extern io *io;

void
timer::timer_reset()
{
	timer_counter = 0;
}

void
timer::timer_step()
{
	if (!(io->reg[rTAC] & 4)) {
		return;
	}

	int divider;
	switch (io->reg[rTAC] & 3) {
		default: // clang is stupid
		case 0:
			divider = 1024;
			break;
		case 1:
			divider = 16;
			break;
		case 2:
			divider = 64;
			break;
		case 3:
			divider = 256;
			break;
	}

	//	printf("TIMER STEP %d %d %d\n", divider, timer_counter, io->reg[rTIMA]);

	if (++timer_counter == divider) {
		timer_counter = 0;
		if (++io->reg[rTIMA] == 0) {
			io->reg[rTIMA] = io->reg[rTMA];
			//			printf("TIMER FIRED %s:%d\n", __FILE__, __LINE__);
			io->reg[rIF] |= 4;
		}
	}
}

uint8_t
timer::timer_read(uint8_t a8)
{
	// these behave like RAM
	return io->reg[a8];
}

void
timer::timer_write(uint8_t a8, uint8_t d8)
{
	switch (a8) {
		case rTAC:
		case rTIMA:
			io->reg[a8] = d8;
			timer_reset();
			break;
		default:
			printf("warning: timer I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);
			io->reg[a8] = d8;
			break;
	}
}

