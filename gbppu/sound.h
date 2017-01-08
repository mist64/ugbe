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
	bool pulse_on[2];
	int pulse_freq[2];
	int pulse_trigger[2];
	bool pulse_invert[2];
	int pulse_freq_counter[2];
	bool pulse_value[2];

	void pulse_restart(int c);

protected:
	friend class gb;
	sound(io &io);

public:
	uint8_t read(uint8_t a8);
	void write(uint8_t a8, uint8_t d8);
	void step();
    void (* consumeSoundInteger)(int16_t);
};

#endif /* sound_h */
