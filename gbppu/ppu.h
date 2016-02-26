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

#endif /* ppu_h */
