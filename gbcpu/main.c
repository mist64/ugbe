//
//  main.c
//  gbcpu
//
//  Created by Lisa Brodner on 2016-02-16.
//  Copyright © 2016 Lisa Brodner. All rights reserved.
//

#include <stdio.h>

int
main(int argc, const char * argv[])
{
	uint8_t RAM[65536];
	uint16_t pc = 0;
	uint16_t sp = 0;
	
	uint8_t a = 0;
	
	// registers, can be used as 16 bit registers in the following combinations:
	// a,f,
	// b,c,
	// d,e,
	// h,l

	
	FILE *f = fopen("/Users/mist/Documents/git/gbcpu/gbcpu/DMG_ROM.bin", "r");
	fread(RAM, 256, 1, f);
	fclose(f);
	
	for (;;) {
		uint8_t opcode = RAM[pc++];
		printf("0x%x\n", opcode);
		
		switch (opcode) {
			case 0: // nop
				break;
				
			case 0x31: { // LD SP,d16 - 3
				uint8_t d16l = RAM[pc++];
				uint8_t d16u = RAM[pc++];
				
				uint16_t d16 = (d16u << 8) + d16l;
//				printf("%x%x %x\n", d16l, d16u, d16);
				sp = d16;
				break;
			}
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
