//
//  timer.h
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef timer_h
#define timer_h

#include <stdint.h>
#include <stdio.h>

class io;

class timer {
private:
	io  &_io;
	int  counter;

private:
	void reset();

protected:
    friend class gb;
	timer(io &io);

public:
	uint8_t read(uint8_t a8);
	void write(uint8_t a8, uint8_t d8);

public:
	void step();
};

#endif /* timer_h */
