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

uint8_t
fetch8()
{
	uint8_t d8 = RAM[pc++];
	return d8;

}

uint16_t
fetch16()
{
	uint8_t d16l = fetch8();
	uint8_t d16h = fetch8();
	uint16_t d16 = (d16h << 8) | d16l;
	return d16;
}

void
ld8(uint8_t *r8)
{
	uint8_t d8 = fetch8();
	*r8 = d8;
}

void
ld16(uint16_t *r16)
{
	uint16_t d16 = fetch16();
	*r16 = d16;
}

void
push8(uint8_t d8)
{
	RAM[--sp] = d8;
}

void
push16(uint16_t d16)
{
	push8(d16 >> 8);
	push8(d16 & 0xff);
}

uint8_t
pop8()
{
	uint8_t d8 = RAM[sp++];
	return d8;
}

uint16_t
pop16()
{
	uint8_t d16l = pop8();
	uint8_t d16h = pop8();
	uint16_t d16 = (d16h << 8) | d16l;
	return d16;
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
		
		uint8_t opcode = fetch8();
		printf("0x%x\n", opcode);
		
		switch (opcode) {
			case 0x00: // nop; 1; ----
				break;
			case 0x01: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x02: // LD (BC),A; 1; ----
				RAM[bc] = a;
				break;
			case 0x03: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x04: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x05: // DEC B; 1; Z 1 H -
				b--;
				zf = !b;
				nf = 1;
//				hf = ; // todo: calculate hf
				break;
			case 0x06: // LD B,d8; 2; ----
				ld8(&b);
				break;
			case 0x07: { // RLCA; 1; 0 0 0 C:
				cf = a >> 7;
				a = a << 1;
				zf = 0;
				nf = 0;
				hf = 0;
				break;
			}
			case 0x08: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x09: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x0a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x0b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x0c: // INC C; 1; Z 0 H -
				c++;
				zf = !c;
				nf = 0;
//				hf = ; // todo: calculate hf
				break;
			case 0x0d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x0e: // LD C,d8; 2; ----
				ld8(&c);
				break;
			case 0x0f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x10: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x11: // LD DE,d16; 3; ----
				ld16(&de);
				break;
			case 0x12: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x13: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x14: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x15: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x16: // LD D,d8; 2; ----
				ld8(&d);
				break;
			case 0x17: { // RLA; 1; 0 0 0 C:
				uint8_t oldcf = cf;
				cf = a >> 7;
				a = (a << 1) | oldcf;
				zf = 0;
				nf = 0;
				hf = 0;
				break;
			}
			case 0x18: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x19: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x1a: // LD A,(DE); 1; ----
				a = RAM[de];
				break;
			case 0x1b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x1c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x1d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x1e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x1f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x20: { // JR NZ, r8;2; ----
				int8_t r8 = fetch8();
				if (zf == 0) {
					pc += r8;
				}
				break;
			}
            case 0x21: // LD HL,d16; 3; ----
				ld16(&hl);
				break;
			case 0x22: // LD (HL+),A; 1; ---- // LD (HLI),A or LDI (HL),A // target, source
				RAM[hl++] = a;
				break;
			case 0x23: // INC HL; 1; ----
				hl++;
				break;
			case 0x24: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x25: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x26: // LD H,d8; 2; ----
				ld8(&h);
				break;
			case 0x27: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x28: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x29: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x2f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x30: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x31: // LD SP,d16; 3; ----
				ld16(&sp);
				break;
			case 0x32: // LD (HL-),A; 1; ---- // LD (HLD),A or LDD (HL),A // target, source
				RAM[hl--] = a;
				break;
			case 0x33: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x34: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x35: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x36: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x37: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x38: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x39: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x3a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x3b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x3c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x3d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x3e: // LD A,d8; 2; ----
				ld8(&a);
				break;
			case 0x3f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x40: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x41: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x42: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x43: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x44: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x45: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x46: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x47: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x48: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x49: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x4f: // LD C,A; 1; ----
				c = a;
				break;
			case 0x50: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x51: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x52: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x53: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x54: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x55: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x56: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x57: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x58: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x59: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x5f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x60: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x61: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x62: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x63: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x64: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x65: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x66: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x67: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x68: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x69: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x6f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;

			case 0x70: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x71: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x72: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x73: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x74: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x75: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x76: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x77: // LD (HL),A; 1; ----
				RAM[hl] = a;
				break;
			case 0x78: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x79: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x7f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x80: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x81: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x82: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x83: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x84: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x85: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x86: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x87: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x88: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x89: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x8f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x90: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x91: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x92: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x93: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x94: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x95: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x96: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x97: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x98: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x99: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9a: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9b: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9c: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9d: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9e: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0x9f: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa0: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa1: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa2: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa5: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xa9: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xaa: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xab: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xac: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xad: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xae: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xaf: // XOR A - 1; 1; Z000 // XOR A,A
				a = 0;
				break;
			case 0xb0: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb1: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb2: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb5: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xb9: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xba: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xbb: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xbc: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xbd: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xbe: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xbf: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc0: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc1: // POP BC; 1; ----
				bc = pop16();
				break;
			case 0xc2: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc5: // PUSH BC; 1; ----
				push16(bc);
				break;
			case 0xc6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xc9: // RET; 1; ----
				pc = pop16();
				break;
			case 0xca: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			
			case 0xcb: // PREFIX CB
				opcode = fetch8();
				printf("0x%02x\n", opcode);

				switch (opcode) {
					case 0x11: { // RL C; 2; Z 0 0 C
						uint8_t oldcf = cf;
						cf = c >> 7;
						c = (c << 1) | oldcf;
						zf = !c;
						nf = 0;
						hf = 0;
						break;
					}
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

			case 0xcc: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xcd: { // CALL a16; 3; ----
				uint16_t a16 = fetch16();
				push16(pc);
				pc = a16;
				break;
			}
			case 0xce: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xcf: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd0: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd1: // POP DE; 1; ----
				de = pop16();
				break;
			case 0xd2: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd5: // PUSH DE; 1; ----
				push16(de);
				break;
			case 0xd6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xd9: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xda: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xdb: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xdc: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xdd: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xde: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xdf: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe0: { // LDH (a8),A; 2; ---- // LD ($FF00+a8),A
				uint8_t a8 = fetch8();
				RAM[0xff00 + a8] = a;
				break;
			}
			case 0xe1: // POP HL; 1; ----
				hl = pop16();
				break;
			case 0xe2: // LD (C),A; 1; ---- // LD ($FF00+C),A // target, source
				RAM[0xff00 + c] = a;
				break;
			case 0xe3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe5: // PUSH HL; 1; ----
				push16(hl);
				break;
			case 0xe6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xe9: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xea: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xeb: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xec: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xed: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xee: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xef: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf0: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf1: // POP AF; 1; Z N H C
				af = pop16();
				break;
			case 0xf2: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf3: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf4: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf5: // PUSH AF; 1; ----
				push16(af);
				break;
			case 0xf6: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf7: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf8: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xf9: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xfa: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xfb: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xfc: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xfd: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xfe: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;
			case 0xff: //
				printf("todo: 0x%x\n", opcode);
				return 1; // todo
				break;

			default:
				printf("Unknown Opcode 0x%02x\n", opcode);
				return 1;
		}
	}
	
	return 0;
}
