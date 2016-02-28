//
//  memory.h
//  gbppu
//
//  Created by Michael Steil on 2016-02-28
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef memory_h
#define memory_h

#include <stdio.h>

void mem_init();
uint8_t mem_read(uint16_t a16);
void mem_write(uint16_t a16, uint8_t d8);

#endif /* memory_h */