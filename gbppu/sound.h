//
//  sound.h
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef sound_h
#define sound_h

#include <stdint.h>
#include <stdio.h>

class io;

class sound {
private:
	io &_io;
	int clock_divider;
	bool c1_on;
	int c1_freq;
	int c1_trigger;
	bool c1_invert;
	int c1_freq_counter;
	bool c1_value;

	void c1_restart();

protected:
	friend class gb;
	sound(io &io);

public:
	uint8_t read(uint8_t a8);
	void write(uint8_t a8, uint8_t d8);
	void step();
};

#endif /* sound_h */
