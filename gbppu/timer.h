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

class timer {
private:
	int timer_counter;

	void timer_reset();

public:
	uint8_t timer_read(uint8_t a8);
	void timer_write(uint8_t a8, uint8_t d8);

	void timer_step();
};

#endif /* timer_h */
