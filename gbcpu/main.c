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

union reg16_t reg16_hl;

#define hl reg16_hl.full
#define h reg16_hl.high
#define l reg16_hl.low

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

#if BUILD_USER_Lisa
	FILE *f = fopen("/Users/lisa/Projects/gbcpu/gbcpu/DMG_ROM.bin", "r");
#else
    FILE *f = fopen("/Users/mist/Documents/git/gbcpu/gbcpu/DMG_ROM.bin", "r");
#endif

    fread(RAM, 256, 1, f);
	fclose(f);
	
	for (;;) {
		printf("A=%02x HL=%04x SP=%04x PC=%04x\n", a, hl, sp, pc);
		
		uint8_t opcode = RAM[pc++];
		printf("0x%x\n", opcode);
		
		switch (opcode) {
			case 0x0: // nop
				break;
				
			case 0x21: // LD HL,d16 - 3
				ld16(&hl);
				break;
			case 0x31: // LD SP,d16 - 3
				ld16(&sp);
				break;
			case 0x32: // LD (HL-),A // target, source
				RAM[hl--] = a;
				break;

			case 0xaf: // XOR A - 1 // XOR A,A
				a = 0;
				break;
				
			case 0xcb: // PREFIX CB
				opcode = RAM[pc++];
				printf("0x%x\n", opcode);

				switch (opcode) {
					case 0x7c: // BIT 7,H
						
						break;
						
					default:
						printf("Unknown Prefix CB Opcode\n");
						return 1;
						break;
				}
			default:
				printf("Unknown Opcode\n");
				return 1;
		}
	}
	
	return 0;
}
