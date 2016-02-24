//
//  main.c
//  gbcpu
//
//  Created by Lisa Brodner on 2016-02-16.
//  Copyright © 2016 Lisa Brodner. All rights reserved.
//

#include <stdio.h>


// see: http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// see: http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM
// see: http://meatfighter.com/gameboy/GBCribSheet000129.pdf


uint8_t RAM[65536];
uint16_t pc = 0;
uint16_t sp = 0;


// registers, can be used as 16 bit registers in the following combinations:
// a,f,
// b,c,
// d,e,
// h,l

// f - flags register in detail
// 7	6	5	4	3	2	1	0
// Z	N	H	C	0	0	0	0
// Z - Zero Flag
// N - Subtract Flag
// H - Half Carry Flag - did a carry from 3 to 4 bit happen
// C - Carry Flag
// 0 - Not used, always zero

#pragma pack(1)
union reg16_t {
	uint16_t full;
	struct {
        union {
            uint8_t full;
            struct {
                unsigned int bit7 : 1;
                unsigned int bit6 : 1;
                unsigned int bit5 : 1;
                unsigned int bit4 : 1;
                unsigned int bit3_0 : 4;
            };
        } low;
		uint8_t high;
	};
};

union reg16_t reg16_af;
union reg16_t reg16_bc;
union reg16_t reg16_de;
union reg16_t reg16_hl;

#define af reg16_af.full
#define a reg16_af.high
#define f reg16_af.low.full

#define zf reg16_af.low.bit7
#define nf reg16_af.low.bit6
#define hf reg16_af.low.bit5
#define cf reg16_af.low.bit4

#define bc reg16_bc.full
#define b reg16_bc.high
#define c reg16_bc.low.full

#define de reg16_de.full
#define d reg16_de.high
#define e reg16_de.low.full

#define hl reg16_hl.full
#define h reg16_hl.high
#define l reg16_hl.low.full

void
ld8(uint8_t *r8)
{
	uint8_t d8 = RAM[pc++];
	*r8 = d8;
}

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
	FILE *file = fopen("/Users/lisa/Projects/gbcpu/gbcpu/DMG_ROM.bin", "r");
#else
    FILE *file = fopen("/Users/mist/Documents/git/gbcpu/gbcpu/DMG_ROM.bin", "r");
#endif

    fread(RAM, 256, 1, file);
	fclose(file);
	
	for (;;) {
		printf("A=%02x BC=%04x HL=%04x SP=%04x PC=%04x (ZF=%d,NF=%d,HF=%d,CF=%d)\n", a, bc, hl, sp, pc, zf, nf, hf, cf);
		
		uint8_t opcode = RAM[pc++];
		printf("0x%x\n", opcode);
		
		switch (opcode) {
			case 0x00: // nop; 1; ----
				break;
//			case 0x02: // LD (BC),A; 1; ----
//				RAM[bc] = a;
//				break;
			case 0x0c: // INC C; 1; Z 0 H -
				c++;
				zf = !c;
				nf = 0;
//				hf = ; // todo: calculate hf
				break;
			case 0x0e: // LD C,d8; 2; ----
				ld8(&c);
				break;
			case 0x11: // LD DE,d16; 3; ----
				ld16(&de);
				break;
			case 0x1a: // LD A,(DE); 1; ----
				a = RAM[de];
				break;
			case 0x20: { // JR NZ, r8;2; ----
				int8_t r8 = RAM[pc++];
				if (zf == 0) {
					pc += r8;
				}
				break;
			}
            case 0x21: // LD HL,d16; 3; ----
				ld16(&hl);
				break;
			case 0x31: // LD SP,d16; 3; ----
				ld16(&sp);
				break;
			case 0x32: // LD (HL-),A; 1; ---- // LD (HLD),A or LDD (HL),A // target, source
				RAM[hl--] = a;
				break;
			case 0x3e: // LD A,d8; 2; ----
				ld8(&a);
				break;
			case 0x77: // LD (HL),A; 1; ----
				RAM[hl] = a;
				break;
			case 0xaf: // XOR A - 1; 1; Z000 // XOR A,A
				a = 0;
				break;
				
			case 0xcb: // PREFIX CB
				opcode = RAM[pc++];
				printf("0x%02x\n", opcode);

				switch (opcode) {
                    case 0x7c: { // BIT 7,H; 2; Z01-
                        uint8_t test = 1 << 7;
						zf = !(h & test);
                        nf = 0;
                        hf = 1;
                        break;
                    }
						
					default:
						printf("Unknown Prefix CB Opcode 0x%02x\n", opcode);
						return 1;
						break;
				}
                break;
			case 0xcd: // CALL a16; 3; ----
				// get address
				// pc to stack
				// jump there
				// todo: implement
				return 1;

				break;
			case 0xe0: { // LDH (a8),A; 2; ---- // LD ($FF00+a8),A
				uint8_t a8 = RAM[pc++];
				RAM[0xff00 + a8] = a;
				break;
			}
			case 0xe2: // LD (C),A; 1; ---- // LD ($FF00+C),A // target, source
				RAM[0xff00 + c] = a;
				break;

			default:
				printf("Unknown Opcode 0x%02x\n", opcode);
				return 1;
		}
	}
	
	return 0;
}
