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
mem_read(uint16_t a16)
{
	return RAM[a16];
}

uint8_t
fetch8()
{
	uint8_t d8 = mem_read(pc++);
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
	uint8_t d8 = mem_read(sp++);
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

void
inc8(uint8_t *r8) // INC \w 1; 4; Z 0 H -
{
	(*r8)++;
	zf = !*r8;
	nf = 0;
//	hf = ; // todo: calculate hf
}

void
dec8(uint8_t *r8) // DEC \w 1; 4; Z 1 H -
{
	(*r8)--;
	zf = !*r8;
	nf = 1;
//	hf = ; // todo: calculate hf
}

void
cpa8(uint8_t d8) // CP \w; 1; 4; Z 1 H C
{
	zf = (a == d8);
	nf = 1;
//	hf = ; // todo: calculate hf
	cf = a < d8;
}

void
jrcc(int condition) // jump relative condition code
{
	int8_t r8 = fetch8();
	if (condition) {
		pc += r8;
	}

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
		printf("0x%02x\n", opcode);
		
		switch (opcode) {
			case 0x00: // NOP; 1; 4; ----
				break;
			case 0x01: // LD BC,d16; 3; 12; ----
				ld16(&bc);
				break;
			case 0x02: // LD (BC),A; 1; 8; ----
				RAM[bc] = a;
				break;
			case 0x03: // INC BC; 1; 8; ----
				bc++;
				break;
			case 0x04: // INC B; 1; 4; Z 0 H -
				inc8(&b);
				break;
			case 0x05: // DEC B; 1; 4; Z 1 H -
				dec8(&b);
				break;
			case 0x06: // LD B,d8; 2; 8; ----
				ld8(&b);
				break;
			case 0x07: { // RLCA; 1; 4; 0 0 0 C
				cf = a >> 7;
				a = a << 1;
				zf = 0;
				nf = 0;
				hf = 0;
				break;
			}
			case 0x08: // LD (a16),SP; 3; 20; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x09: // ADD HL,BC; 1; 8; - 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x0a: // LD A,(BC); 1; 8; ----
				a = mem_read(bc);
				break;
			case 0x0b: // DEC BC; 1; 8; ----
				bc--;
				break;
			case 0x0c: // INC C; 1; 4; Z 0 H -
				inc8(&c);
				break;
			case 0x0d: // DEC C; 1; 4; Z 1 H -
				dec8(&c);
				break;
			case 0x0e: // LD C,d8; 2; 8; ----
				ld8(&c);
				break;
			case 0x0f: // RRCA; 1; 4; 0 0 0 C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x10: // STOP 0; 2; 4; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x11: // LD DE,d16; 3; 12; ----
				ld16(&de);
				break;
			case 0x12: // LD (DE),A; 1; 8; ----
				RAM[de] = a;
				break;
			case 0x13: // INC DE; 1; 8; ----
				de++;
				break;
			case 0x14: // INC D; 1; 4; Z 0 H -
				inc8(&d);
				break;
			case 0x15: // DEC D; 1; 4; Z 1 H -
				dec8(&d);
				break;
			case 0x16: // LD D,d8; 2; 8; ----
				ld8(&d);
				break;
			case 0x17: { // RLA; 1; 4; 0 0 0 C
				uint8_t oldcf = cf;
				cf = a >> 7;
				a = (a << 1) | oldcf;
				zf = 0;
				nf = 0;
				hf = 0;
				break;
			}
			case 0x18: // JR r8; 2; 12; ----
				jrcc(1);
				break;
			case 0x19: // ADD HL,DE; 1; 8; - 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x1a: // LD A,(DE); 1; 8; ----
				a = mem_read(de);
				break;
			case 0x1b: // DEC DE; 1; 8; ----
				de--;
				break;
			case 0x1c: // INC E; 1; 4; Z 0 H -
				inc8(&e);
				break;
			case 0x1d: // DEC E; 1; 4; Z 1 H -
				dec8(&e);
				break;
			case 0x1e: // LD E,d8; 2; 8; ----
				ld8(&e);
				break;
			case 0x1f: // RRA; 1; 4; 0 0 0 C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x20: { // JR NZ,r8; 2; 12/8; ----
				jrcc(!zf);
				break;
			}
            case 0x21: // LD HL,d16; 3; 12; ----
				ld16(&hl);
				break;
			case 0x22: // LD (HL+),A; 1; 8; ---- // LD (HLI),A or LDI (HL),A // target, source
				RAM[hl++] = a;
				break;
			case 0x23: // INC HL; 1; 8; ----
				hl++;
				break;
			case 0x24: // INC H; 1; 4; Z 0 H -
				inc8(&h);
				break;
			case 0x25: // DEC H; 1; 4; Z 1 H -
				dec8(&h);
				break;
			case 0x26: // LD H,d8; 2; 8; ----
				ld8(&h);
				break;
			case 0x27: // DAA; 1; 4; Z - 0 C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x28: // JR Z,r8; 2; 12/8; ----
				jrcc(zf);
				break;
			case 0x29: // ADD HL,HL; 1; 8; - 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x2a: // LD A,(HL+); 1; 8; ----
				a = mem_read(hl++);
				break;
			case 0x2b: // DEC HL; 1; 8; ----
				hl--;
				break;
			case 0x2c: // INC L; 1; 4; Z 0 H -
				inc8(&l);
				break;
			case 0x2d: // DEC L; 1; 4; Z 1 H -
				dec8(&l);
				break;
			case 0x2e: // LD L,d8; 2; 8; ----
				ld8(&l);
				break;
			case 0x2f: // CPL; 1; 4; - 1 1 -
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x30: // JR NC,r8; 2; 12/8; ----
				jrcc(!cf);
				break;
			case 0x31: // LD SP,d16; 3; 12; ----
				ld16(&sp);
				break;
			case 0x32: // LD (HL-),A; 1; 8; ---- // LD (HLD),A or LDD (HL),A // target, source
				RAM[hl--] = a;
				break;
			case 0x33: // INC SP; 1; 8; ----
				sp++;
				break;
			case 0x34: // INC (HL); 1; 12; Z 0 H -
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x35: // DEC (HL); 1; 12; Z 1 H -
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x36: // LD (HL),d8; 2; 12; ----
				RAM[hl] = fetch8();
				break;
			case 0x37: // SCF; 1; 4; - 0 0 1
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x38: // JR C,r8; 2; 12/8; ----
				jrcc(cf);
				break;
			case 0x39: // ADD HL,SP; 1; 8; - 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x3a: // LD A,(HL-); 1; 8; ----
				a = mem_read(hl--);
				break;
			case 0x3b: // DEC SP; 1; 8; ----
				sp--;
				break;
			case 0x3c: // INC A; 1; 4; Z 0 H -
				inc8(&a);
				break;
			case 0x3d: // DEC A; 1; 4; Z 1 H -
				dec8(&a);
				break;
			case 0x3e: // LD A,d8; 2; 8; ----
				ld8(&a);
				break;
			case 0x3f: // CCF; 1; 4; - 0 0 C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x40: // LD B,B; 1; 4; ----
				b = b;
				break;
			case 0x41: // LD B,C; 1; 4; ----
				b = c;
				break;
			case 0x42: // LD B,D; 1; 4; ----
				b = d;
				break;
			case 0x43: // LD B,E; 1; 4; ----
				b = e;
				break;
			case 0x44: // LD B,H; 1; 4; ----
				b = h;
				break;
			case 0x45: // LD B,L; 1; 4; ----
				b = l;
				break;
			case 0x46: // LD B,(HL); 1; 8; ----
				b = mem_read(hl);
				break;
			case 0x47: // LD B,A; 1; 4; ----
				b = a;
				break;
			case 0x48: // LD C,B; 1; 4; ----
				c = b;
				break;
			case 0x49: // LD C,C; 1; 4; ----
				c = c;
				break;
			case 0x4a: // LD C,D; 1; 4; ----
				c = d;
				break;
			case 0x4b: // LD C,E; 1; 4; ----
				c = e;
				break;
			case 0x4c: // LD C,H; 1; 4; ----
				c = h;
				break;
			case 0x4d: // LD C,L; 1; 4; ----
				c = l;
				break;
			case 0x4e: // LD C,(HL); 1; 8; ----
				c = mem_read(hl);
				break;
			case 0x4f: // LD C,A; 1; 4; ----
				c = a;
				break;
			case 0x50: // LD D,B; 1; 4; ----
				d = b;
				break;
			case 0x51: // LD D,C; 1; 4; ----
				d = c;
				break;
			case 0x52: // LD D,D; 1; 4; ----
				d = d;
				break;
			case 0x53: // LD D,E; 1; 4; ----
				d = e;
				break;
			case 0x54: // LD D,H; 1; 4; ----
				d = h;
				break;
			case 0x55: // LD D,L; 1; 4; ----
				d = l;
				break;
			case 0x56: // LD D,(HL); 1; 8; ----
				d = mem_read(hl);
				break;
			case 0x57: // LD D,A; 1; 4; ----
				d = a;
				break;
			case 0x58: // LD E,B; 1; 4; ----
				e = b;
				break;
			case 0x59: // LD E,C; 1; 4; ----
				e = c;
				break;
			case 0x5a: // LD E,D; 1; 4; ----
				e = d;
				break;
			case 0x5b: // LD E,E; 1; 4; ----
				e = e;
				break;
			case 0x5c: // LD E,H; 1; 4; ----
				e = h;
				break;
			case 0x5d: // LD E,L; 1; 4; ----
				e = l;
				break;
			case 0x5e: // LD E,(HL); 1; 8; ----
				e = mem_read(hl);
				break;
			case 0x5f: // LD E,A; 1; 4; ----
				e = a;
				break;
			case 0x60: // LD H,B; 1; 4; ----
				h = b;
				break;
			case 0x61: // LD H,C; 1; 4; ----
				h = c;
				break;
			case 0x62: // LD H,D; 1; 4; ----
				h = d;
				break;
			case 0x63: // LD H,E; 1; 4; ----
				h = e;
				break;
			case 0x64: // LD H,H; 1; 4; ----
				h = h;
				break;
			case 0x65: // LD H,L; 1; 4; ----
				h = l;
				break;
			case 0x66: // LD H,(HL); 1; 8; ----
				h = mem_read(hl);
				break;
			case 0x67: // LD H,A; 1; 4; ----
				h = a;
				break;
			case 0x68: // LD L,B; 1; 4; ----
				l = b;
				break;
			case 0x69: // LD L,C; 1; 4; ----
				l = c;
				break;
			case 0x6a: // LD L,D; 1; 4; ----
				l = d;
				break;
			case 0x6b: // LD L,E; 1; 4; ----
				l = e;
				break;
			case 0x6c: // LD L,H; 1; 4; ----
				l = h;
				break;
			case 0x6d: // LD L,L; 1; 4; ----
				l = l;
				break;
			case 0x6e: // LD L,(HL); 1; 8; ----
				l = mem_read(hl);
				break;
			case 0x6f: // LD L,A; 1; 4; ----
				l = a;
				break;
			case 0x70: // LD (HL),B; 1; 8; ----
				RAM[hl] = b;
				break;
			case 0x71: // LD (HL),C; 1; 8; ----
				RAM[hl] = c;
				break;
			case 0x72: // LD (HL),D; 1; 8; ----
				RAM[hl] = d;
				break;
			case 0x73: // LD (HL),E; 1; 8; ----
				RAM[hl] = e;
				break;
			case 0x74: // LD (HL),H; 1; 8; ----
				RAM[hl] = h;
				break;
			case 0x75: // LD (HL),L; 1; 8; ----
				RAM[hl] = l;
				break;
			case 0x76: // HALT; 1; 4; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x77: // LD (HL),A; 1; 8; ----
				RAM[hl] = a;
				break;
			case 0x78: // LD A,B; 1; 4; ----
				a = b;
				break;
			case 0x79: // LD A,C; 1; 4; ----
				a = c;
				break;
			case 0x7a: // LD A,D; 1; 4; ----
				a = d;
				break;
			case 0x7b: // LD A,E; 1; 4; ----
				a = e;
				break;
			case 0x7c: // LD A,H; 1; 4; ----
				a = h;
				break;
			case 0x7d: // LD A,L; 1; 4; ----
				a = l;
				break;
			case 0x7e: // LD A,(HL); 1; 8; ----
				a = mem_read(hl);
				break;
			case 0x7f: // LD A,A; 1; 4; ----
				a = a;
				break;
			case 0x80: // ADD A,B; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x81: // ADD A,C; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x82: // ADD A,D; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x83: // ADD A,E; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x84: // ADD A,H; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x85: // ADD A,L; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x86: // ADD A,(HL); 1; 8; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x87: // ADD A,A; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x88: // ADC A,B; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x89: // ADC A,C; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8a: // ADC A,D; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8b: // ADC A,E; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8c: // ADC A,H; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8d: // ADC A,L; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8e: // ADC A,(HL); 1; 8; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x8f: // ADC A,A; 1; 4; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x90: // SUB B; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x91: // SUB C; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x92: // SUB D; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x93: // SUB E; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x94: // SUB H; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x95: // SUB L; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x96: // SUB (HL); 1; 8; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x97: // SUB A; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x98: // SBC A,B; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x99: // SBC A,C; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9a: // SBC A,D; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9b: // SBC A,E; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9c: // SBC A,H; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9d: // SBC A,L; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9e: // SBC A,(HL); 1; 8; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0x9f: // SBC A,A; 1; 4; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa0: // AND B; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa1: // AND C; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa2: // AND D; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa3: // AND E; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa4: // AND H; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa5: // AND L; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa6: // AND (HL); 1; 8; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa7: // AND A; 1; 4; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa8: // XOR B; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xa9: // XOR C; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xaa: // XOR D; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xab: // XOR E; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xac: // XOR H; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xad: // XOR L; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xae: // XOR (HL); 1; 8; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xaf: // XOR A; 1; 4; Z 0 0 0 // XOR A,A
				a = 0;
				break;
			case 0xb0: // OR B; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb1: // OR C; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb2: // OR D; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb3: // OR E; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb4: // OR H; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb5: // OR L; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb6: // OR (HL); 1; 8; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb7: // OR A; 1; 4; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xb8: // CP B; 1; 4; Z 1 H C
				cpa8(b);
				break;
			case 0xb9: // CP C; 1; 4; Z 1 H C
				cpa8(c);
				break;
			case 0xba: // CP D; 1; 4; Z 1 H C
				cpa8(d);
				break;
			case 0xbb: // CP E; 1; 4; Z 1 H C
				cpa8(e);
				break;
			case 0xbc: // CP H; 1; 4; Z 1 H C
				cpa8(h);
				break;
			case 0xbd: // CP L; 1; 4; Z 1 H C
				cpa8(l);
				break;
			case 0xbe: // CP (HL); 1; 8; Z 1 H C
				cpa8(mem_read(hl));
				break;
			case 0xbf: // CP A; 1; 4; Z 1 H C
				cpa8(a);
				break;
			case 0xc0: // RET NZ; 1; 20/8; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc1: // POP BC; 1; 12; ----
				bc = pop16();
				break;
			case 0xc2: // JP NZ,a16; 3; 16/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc3: // JP a16; 3; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc4: // CALL NZ,a16; 3; 24/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc5: // PUSH BC; 1; 16; ----
				push16(bc);
				break;
			case 0xc6: // ADD A,d8; 2; 8; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc7: // RST 00H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc8: // RET Z; 1; 20/8; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xc9: // RET; 1; 16; ----
				pc = pop16();
				break;
			case 0xca: // JP Z,a16; 3; 16/12; ----
				printf("todo: 0x%02x\n", opcode);
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

			case 0xcc: // CALL Z,a16; 3; 24/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xcd: { // CALL a16; 3; 24; ----
				uint16_t a16 = fetch16();
				push16(pc);
				pc = a16;
				break;
			}
			case 0xce: // ADC A,d8; 2; 8; Z 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xcf: // RST 08H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd0: // RET NC; 1; 20/8; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd1: // POP DE; 1; 12; ----
				de = pop16();
				break;
			case 0xd2: // JP NC,a16; 3; 16/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd3: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xd4: // CALL NC,a16; 3; 24/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd5: // PUSH DE; 1; 16; ----
				push16(de);
				break;
			case 0xd6: // SUB d8; 2; 8; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd7: // RST 10H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd8: // RET C; 1; 20/8; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xd9: // RETI; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xda: // JP C,a16; 3; 16/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xdb: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xdc: // CALL C,a16; 3; 24/12; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xdd: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xde: // SBC A,d8; 2; 8; Z 1 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xdf: // RST 18H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xe0: { // LDH (a8),A; 2; 12; ---- // LD ($FF00+a8),A
				uint8_t a8 = fetch8();
				RAM[0xff00 + a8] = a;
				break;
			}
			case 0xe1: // POP HL; 1; 12; ----
				hl = pop16();
				break;
			case 0xe2: // LD (C),A; 1; 8; ---- // LD ($FF00+C),A // target, source
				RAM[0xff00 + c] = a;
				break;
			case 0xe3: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xe4: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xe5: // PUSH HL; 1; 16; ----
				push16(hl);
				break;
			case 0xe6: // AND d8; 2; 8; Z 0 1 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xe7: // RST 20H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xe8: // ADD SP,r8; 2; 16; 0 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xe9: // JP (HL); 1; 4; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xea: // LD (a16),A; 3; 16; ----
				RAM[fetch16()] = a;
				break;
			case 0xeb: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xec: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xed: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xee: // XOR d8; 2; 8; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xef: // RST 28H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xf0: // LDH A,(a8); 2; 12; ---- // LD A,($FF00+a8)
				a = mem_read(0xff00 + fetch8());
				break;
			case 0xf1: // POP AF; 1; 12; Z N H C
				af = pop16();
				break;
			case 0xf2: // LD A,(C); 1; 8; ---- // LD A,($FF00+C)
				a = mem_read(0xff00 + c);
				break;
			case 0xf3: // DI; 1; 4; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xf4: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xf5: // PUSH AF; 1; 16; ----
				push16(af);
				break;
			case 0xf6: // OR d8; 2; 8; Z 0 0 0
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xf7: // RST 30H; 1; 16; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xf8: // LD HL,SP+r8; 2; 12; 0 0 H C
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xf9: // LD SP,HL; 1; 8; ----
				sp = hl;
				break;
			case 0xfa: // LD A,(a16); 3; 16; ----
				a = mem_read(fetch16());
				break;
			case 0xfb: // EI; 1; 4; ----
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;
			case 0xfc: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xfd: // crash
				printf("crash: 0x%02x\n", opcode);
				return 1;
			case 0xfe: // CP d8; 2; 8; Z 1 H C
				cpa8(fetch8());
				break;
			case 0xff: // RST 38H; 1; 16; ---
				printf("todo: 0x%02x\n", opcode);
				return 1; // todo
				break;

			default:
				printf("Unknown Opcode 0x%02x\n", opcode);
				return 1;
		}
	}
	
	return 0;
}
