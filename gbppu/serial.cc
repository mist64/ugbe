//
//  serial.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include <assert.h>
#include "serial.h"
#include "io.h"

serial::serial(io &io)
	: _io(io)
{
}

uint8_t serial::
read(uint8_t a8)
{
//	printf("warning: serial read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, _io.reg[a8]);
	switch (a8) {
		case rSB:
			return 0x00;
		case rSC:
			return 0x7e;
	}
	assert(0);
}

void serial::
write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;
//	printf("warning: serial I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);
}
