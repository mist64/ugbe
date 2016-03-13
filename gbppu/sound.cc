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
}

uint8_t sound::
read(uint8_t a8)
{
//	printf("warning: sound read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, _io.reg[a8]);
	return 0xff;
}

void sound::
write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;
//	printf("warning: sound I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);
}
