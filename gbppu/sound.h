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

protected:
	friend class gb;
	sound(io &io);

public:
	uint8_t read(uint8_t a8);
	void write(uint8_t a8, uint8_t d8);
};

#endif /* sound_h */
