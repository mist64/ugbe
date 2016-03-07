//
//  serial.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "serial.h"
#include "io.h"

uint8_t
serial_read(uint8_t a8)
{
	printf("warning: serial read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, io[a8]);
	return 0xff;
}

void
serial_write(uint8_t a8, uint8_t d8)
{
	io[a8] = d8;
	printf("warning: serial I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);
}
