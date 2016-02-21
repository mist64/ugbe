//
//  main.c
//  gbcpu
//
//  Created by Lisa Brodner on 2016-02-16.
//  Copyright © 2016 Lisa Brodner. All rights reserved.
//

#include <stdio.h>

uint8_t RAM[65536];
uint16_t pc = 0;
uint16_t sp = 0;

uint8_t a = 0;
uint8_t h = 0;
uint8_t l = 0;

// registers, can be used as 16 bit registers in the following combinations:
// a,f,
// b,c,
// d,e,
// h,l

union reg16_t {
	uint16_t full;
	struct {
		uint8_t low;
		uint8_t high;
	};
};

union reg16_t hl;

void
ld16(uint16_t *r16)
{
	uint8_t d16l = RAM[pc++];
	uint8_t d16h = RAM[pc++];
	*r16 = (d16h << 8) | d16l;
}

int
main(int argc, const char * argv[])
{

	
	FILE *f = fopen("/Users/mist/Documents/git/gbcpu/gbcpu/DMG_ROM.bin", "r");
	fread(RAM, 256, 1, f);
	fclose(f);
	
	for (;;) {
		uint8_t opcode = RAM[pc++];
		printf("0x%x\n", opcode);
		
		switch (opcode) {
			case 0: // nop
				break;
				
			case 0x21: // LD HL,d16 - 3
				ld16(&hl.full);
				break;
			case 0x31: // LD SP,d16 - 3
				ld16(&sp);
				break;
			case 0xaf: // XOR A - 1 // XOR A,A
				a = 0;
				break;
				
			default:
				printf("Unknown Opcode\n");
				return 1;
		}
	}
	
	return 0;
}
