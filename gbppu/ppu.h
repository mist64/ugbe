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

uint8_t io_get_pending_irqs();
void io_clear_pending_irq(uint8_t irq);

uint8_t io_read(uint8_t);
void io_write(uint8_t, uint8_t);
void io_set_keys(uint8_t keys);

#endif /* ppu_h */
