//
//  sound.c
//  gbppu
//
//  Created by Michael Steil on 2016-03-06.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#include "sound.h"
#include "io.h"

FILE *f;

sound::sound(io &io)
	: _io(io)
{
	pulse_freq[0] = 0;
	pulse_freq[1] = 0;
	pulse_freq_counter[0] = 0;
	pulse_freq_counter[1] = 0;
	pulse_value[0] = false;
	pulse_value[1] = false;
	consumeSoundInteger = NULL;

//	f = fopen("/tmp/del.pcm", "w");
}

void sound::
pulse_restart(int c)
{
	switch ((c ? _io.reg[rNR21] : _io.reg[rNR11]) >> 6) {
		case 0:
			pulse_trigger[c] = pulse_freq[c] >> 3;
			pulse_invert[c] = false;
			break;
		case 1:
			pulse_trigger[c] = pulse_freq[c] >> 2;
			pulse_invert[c] = false;
			break;
		case 2:
			pulse_trigger[c] = pulse_freq[c] >> 1;
			pulse_invert[c] = false;
			break;
		case 3:
			pulse_trigger[c] = pulse_freq[c] >> 2;
			pulse_invert[c] = true;
			break;
	}
	pulse_freq_counter[c] = pulse_freq[c];
	pulse_value[c] = pulse_invert[c];
}

uint8_t sound::
read(uint8_t a8)
{
//	printf("warning: sound read %s (0xff%02x) -> 0x%02x\n", name_for_io_reg(a8), a8, _io.reg[a8]);
	return 0xff;
}

void sound::
write(uint8_t a8, uint8_t d8)
{
	_io.reg[a8] = d8;
//	printf("warning: sound I/O write %s 0xff%02x <- 0x%02x\n", name_for_io_reg(a8), a8, d8);

	if (a8 == rNR14 || a8 == rNR24) {
		int c = (a8 == rNR14) ? 0 : 1;
		if (d8 & 0x80) {
			pulse_on[c] = true;
			pulse_freq[c] = ((((c ? _io.reg[rNR24] : _io.reg[rNR14]) & 7) << 8) | (c ? _io.reg[rNR23] : _io.reg[rNR13])) ^ 2047;
			pulse_restart(c);
		}
	}
}

// runs at 131072 Hz
void sound::
step()
{
//	clock_divider = (clock_divider + 1) & 7;
	clock_divider = (clock_divider + 1) & 15;
	if (clock_divider) {
		return;
	}

	for (int c = 0; c <= 1; c++) {
		if (pulse_on[c]) {
			pulse_freq_counter[c]--;
			if (pulse_freq_counter[c] == pulse_trigger[c]) {
				pulse_value[c] = !pulse_invert[c];
			} else if (pulse_freq_counter[c] == 0) {
				pulse_restart(c);
			}
		}
	}

	uint16_t mixed_value = (pulse_value[0] ? 8192 : 0) + (pulse_value[1] ? 8192 : 0);
//	fwrite(&mixed_value, 2, 1, f);

	if (this->consumeSoundInteger) {
		// stereo sample
		(*this->consumeSoundInteger)(mixed_value);
		//            (*this->consumeSoundInteger)(pulse_value ? INT16_MAX : INT16_MIN);
	}
}
