//
//  gb.h
//  gbppu
//
//  Created by Orlando Bassotto on 2016-03-13.
//  Copyright © 2016 Orlando Bassotto. All rights reserved.
//

#ifndef gb_h
#define gb_h

#include "buttons.h"
#include "cpu.h"
#include "io.h"
#include "memory.h"
#include "ppu.h"
#include "serial.h"
#include "sound.h"
#include "timer.h"

class gb {
private:
	ppu     _ppu;
	cpu     _cpu;
	memory  _memory;
	io      _io;
	timer   _timer;
	buttons _buttons;
	serial  _serial;

public:
    sound   _sound;
	gb(const char *bootrom_filename, const char *cartridge_filename);

public:
    int step();

public:
    inline void set_buttons(uint8_t keys)
    { _buttons.set(keys); }

public:
    inline bool is_ppu_dirty() const
    { return _ppu.dirty; }
    inline void clear_ppu_dirty()
    { _ppu.dirty = false; }
    uint8_t *copy_ppu_picture(size_t &size);
    uint8_t *copy_tilemap(size_t &tilemap);
};

#endif  /* !gb_h */
