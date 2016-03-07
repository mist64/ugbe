//
//  serial.h
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef serial_h
#define serial_h

#include <stdio.h>

uint8_t serial_read(uint8_t a8);
void serial_write(uint8_t a8, uint8_t d8);

#endif /* serial_h */
