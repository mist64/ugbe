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

class io;

class buttons {
private:
	io      &_io;
	uint8_t  _buttons;

protected:
	friend class gb;
	buttons(io &io);

public:
	uint8_t read();
	void write(uint8_t a8, uint8_t d8);

public:
	void set(uint8_t buttons);
};


#endif /* buttons_h */
