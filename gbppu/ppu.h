//
//  ppu.h
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef ppu_h
#define ppu_h

#include <stdio.h>

extern uint8_t picture[160][144];
extern int ppu_dirty;

void ppu_init();
void ppu_step();

uint8_t ppu_read(uint8_t a8);
void ppu_write(uint8_t a8, uint8_t d8);

uint8_t io_read(uint8_t);
void io_write(uint8_t, uint8_t);

#endif /* ppu_h */
