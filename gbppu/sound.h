//
//  sound.h
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef sound_h
#define sound_h

#include <stdio.h>

uint8_t sound_read(uint8_t a8);
void sound_write(uint8_t a8, uint8_t d8);

#endif /* sound_h */