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

extern uint8_t picture[144][160];
extern int ppu_dirty;

void ppu_init();
void ppu_step();

uint8_t ppu_io_read(uint8_t a8);
void ppu_io_write(uint8_t a8, uint8_t d8);

uint8_t ppu_vram_read(uint16_t a16);
void ppu_vram_write(uint16_t a16, uint8_t d8);
uint8_t ppu_oamram_read(uint8_t a8);
void ppu_oamram_write(uint8_t a8, uint8_t d8);

#endif /* ppu_h */
