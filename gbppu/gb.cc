//
//  gb.cc
//  gbppu
//
//  Created by Orlando Bassotto on 2016-03-13.
//  Copyright © 2016 Orlando Bassotto. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "gb.h"

gb::gb()
	: _ppu    (_memory, _io)
	, _cpu    (_memory, _io)
	, _memory (_ppu, _io)
	, _io     (_ppu, _memory, _timer, _serial, _buttons, _sound)
	, _timer  (_io)
	, _serial (_io)
	, _buttons(_io)
	, _sound  (_io)
{
}

int gb::
step()
{
#if 0
	_ppu.step();
	return 0;
#else
	return _cpu.step();
#endif
}

uint8_t *gb::
copy_ppu_picture(size_t &pictureSize)
{
	pictureSize = sizeof(_ppu.picture);
	uint8_t *pictureCopy = (uint8_t *)malloc(pictureSize);
	memcpy(pictureCopy, _ppu.picture, pictureSize);
	return pictureCopy;
}
