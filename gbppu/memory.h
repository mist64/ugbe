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
void mem_init_filenames(char *bootrom, char *cartridge);
uint8_t mem_read(uint16_t a16);
void mem_write(uint16_t a16, uint8_t d8);

void mem_io_write(uint8_t a8, uint8_t d8);

int mem_is_bootrom_enabled();

#endif /* memory_h */
