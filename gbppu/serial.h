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

class io;

class serial {
private:
	io &_io;

protected:
	friend class gb;
	serial(io &io);

public:
	uint8_t read(uint8_t a8);
	void write(uint8_t a8, uint8_t d8);
};

#endif /* serial_h */
