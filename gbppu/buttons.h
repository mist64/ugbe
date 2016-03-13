//
//  buttons.h
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef buttons_h
#define buttons_h

#include <stdint.h>
#include <stdio.h>

uint8_t buttons_read();
void buttons_write(uint8_t a8, uint8_t d8);

void buttons_set(uint8_t buttons);

#endif /* buttons_h */
