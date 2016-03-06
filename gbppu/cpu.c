//
//  cpu.c
//  gbcpu
//
//  Created by Lisa Brodner on 2016-02-16.
//  Copyright © 2016 Lisa Brodner. All rights reserved.
//

#include "cpu.h"

#include <stdio.h>
#include "memory.h"
#include <assert.h>
#include "ppu.h"

// see references:

// - bootstrap rom
// http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM

// - gb instructions
// http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// http://meatfighter.com/gameboy/GBCribSheet000129.pdf
// http://gameboy.mongenel.com/dmg/opcodes.html

// - z80 instructions
// http://www.z80.info/z80code.htm
// http://z80-heaven.wikidot.com/instructions-set


uint16_t pc = 0;
uint16_t sp = 0;

int interrupts_enabled = 0;

#pragma mark - Registers

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
				unsigned int bit3_0 : 4;
                unsigned int bit4 : 1;
				unsigned int bit5 : 1;
				unsigned int bit6 : 1;
				unsigned int bit7 : 1;
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


#pragma mark - Helper

uint8_t
calc_carry(uint8_t bit, uint16_t da, uint16_t db, uint8_t substraction)
{
	uint32_t bb = 1 << bit;
	uint32_t mask = bb - 1;
	
	uint8_t result;
	if (substraction) {
		result = (da & mask) < (db & mask);
		
	} else {
	 result = ((da & mask) + (db & mask)) >= bb;
	}
	return result;
}

void
set_hf_nf(uint8_t bit, uint16_t da, uint16_t db, uint8_t neg) // set the half carry flag
{
	nf = neg;
	hf = calc_carry(bit, da, db, neg);
	// the neg flag is set every time this is a substraction, so this is used as the substraction flag
}

void
set_cf(uint8_t bit, uint16_t da, uint16_t db, uint8_t substraction)
{
	// todo: check addsp, ldhlsp (relative data)
	cf = calc_carry(bit, da, db, substraction);
}


#pragma mark

static uint8_t
fetch8()
{
	uint8_t d8 = mem_read(pc++);
	return d8;

}

static uint16_t
fetch16()
{
	uint8_t d16l = fetch8();
	uint8_t d16h = fetch8();
	uint16_t d16 = (d16h << 8) | d16l;
	return d16;
}

static void
ldhlsp(uint8_t d8) //  LD HL,SP+r8; 2; 12; 0 0 H C // LDHL SP,r8
{
	uint16_t d16 = (uint16_t)(int8_t)d8;
	hl = sp + d16;
	zf = 0;
	set_hf_nf(4, sp, d16, 0);
	set_cf(8, sp, d16, 0);
}

static void
push8(uint8_t d8)
{
	mem_write(--sp, d8);
}

static void
push16(uint16_t d16)
{
	push8(d16 >> 8);
	push8(d16 & 0xff);
}

static uint8_t
pop8()
{
	uint8_t d8 = mem_read(sp++);
	return d8;
}

static uint16_t
pop16()
{
	uint8_t d16l = pop8();
	uint8_t d16h = pop8();
	uint16_t d16 = (d16h << 8) | d16l;
	return d16;
}


#pragma mark

static void
inc8(uint8_t *r8) // INC \w 1; 4; Z 0 H -
{
	uint8_t old_r8 = *r8;
	(*r8)++;
	zf = !*r8;
	set_hf_nf(4, old_r8, 1, 0);
}

static void
dec8(uint8_t *r8) // DEC \w 1; 4; Z 1 H -
{
	uint8_t old_r8 = *r8;
	(*r8)--;
	zf = !*r8;
	set_hf_nf(4, old_r8, 1, 1);
}

static void
cpa8(uint8_t d8) // CP \w; 1; 4; Z 1 H C
{
	// todo: make cp behave like suba
	zf = (a == d8);
	set_hf_nf(4, a, d8, 1);
	set_cf(8, a, d8, 1);
}

static void
suba8(uint8_t d8) // SUB \w; 1; 4; Z 1 H C
{
	zf = (a == d8);
	set_hf_nf(4, a, d8, 1);
	set_cf(8, a, d8, 1);
	a = a - d8;
}

static void
sbca8(uint8_t d8) // SBC A,\w; 1; 4; Z 1 H C
{
	uint8_t addend = cf + d8;
	set_hf_nf(4, a, addend, 1);
	set_cf(8, a, addend, 1);
	a = a - addend;
	zf = !a;
}

static void
adda8(uint8_t d8) // ADD \w; 1; 8; Z 0 H C
{
	set_hf_nf(4, a, d8, 0);
	set_cf(8, a, d8, 0);
	a = a + d8;
	zf = !a;
}

static void
adca8(uint8_t d8) // ADC A,\w; 1; 4; Z 0 H C
{
	uint8_t addend = d8 + cf;
	set_hf_nf(4, a, addend, 0);
	set_cf(8, a, addend, 0);
	a += addend;
	zf = !a;
}

static void
addhl(uint16_t d16) // ADD HL,\w\w; 1; 8; - 0 H C
{
	set_hf_nf(12, hl, d16, 0);
	set_cf(16, hl, d16, 0);
	hl = hl + d16;
}

static void
addsp(uint8_t d8) // ADD SP,r8; 2; 16; 0 0 H C
{
	uint16_t d16 = (uint16_t)(int8_t)d8;
	set_hf_nf(4, sp, d16, 0);
	set_cf(8, sp, d16, 0);
	sp = sp + d16;
	zf = 0;
}


#pragma mark - Jumping

static void
jrcc(int condition) // jump relative condition code
{
	int8_t r8 = fetch8();
	if (condition) {
		pc += r8;
	}
}

static void
jpcc(int condition) // jump condition code
{
	int16_t d16 = fetch16();
	if (condition) {
		pc = d16;
	}
}

static void
rst8(uint8_t d8)
{
	push16(pc);
	pc = d8;
}

static void
retcc(int condition)
{
	if (condition) {
		pc = pop16();
	}
}

static void
callcc(int condition)
{
	uint16_t a16 = fetch16();
	if (condition) {
		push16(pc);
		pc = a16;
	}
}


#pragma mark - Bitwise Boolean Operations

static void
anda(uint8_t d8) // AND \w; 1; 4; Z 0 1 0
{
	a = a & d8;
	zf = !a;
	nf = 0;
	hf = 1;
	cf = 0;
}

static void
ora(uint8_t d8) // OR \w; 1; 4; Z 0 0 0
{
	a = a | d8;
	zf = !a;
	nf = 0;
	hf = 0;
	cf = 0;
}

static void
xora(uint8_t d8) // XOR \w; 1; 4; Z 0 0 0
{
	a = a ^ d8;
	zf = !a;
	nf = 0;
	hf = 0;
	cf = 0;
}


#pragma mark - Shifting

static void
rl8(uint8_t *r8) // RL \w; 2; 8; Z 0 0 C
{
	uint8_t old_cf = cf;
	cf = *r8 >> 7;
	*r8 = (*r8 << 1) | old_cf;
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
rlc8(uint8_t *r8) // RLC \w; 2; 8; Z 0 0 C
{
	cf = *r8 >> 7;
	*r8 = (*r8 << 1) | cf;
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
rr8(uint8_t *r8) // RR \w; 2; 8; Z 0 0 C
{
	uint8_t old_cf = cf;
	cf = *r8 & 1;
	*r8 = (old_cf << 7) | (*r8 >> 1);
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
rrc8(uint8_t *r8) // RRC \w; 2; 8; Z 0 0 C
{
	cf = *r8 & 1;
	*r8 = (cf << 7) | (*r8 >> 1);
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
sla8(uint8_t *r8) // SLA \w; 2; 8; Z 0 0 C
{
	cf = *r8 >> 7;
	*r8 =  *r8 << 1;
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
sra8(uint8_t *r8) // SRA \w; 2; 8; Z 0 0 0
{
	uint8_t old_bit7 = *r8 & (1 << 7);
	cf = *r8 & 1;
	*r8 = (*r8 >> 1) | old_bit7;
	zf = !*r8;
	nf = 0;
	hf = 0;
}

static void
srl8(uint8_t *r8) // SRL \w; 2; 8; Z 0 0 C
{
	cf = *r8 & 1;
	*r8 = *r8 >> 1;
	zf = !*r8;
	nf = 0;
	hf = 0;
}


#pragma mark - Changing Bits

static void
swap8(uint8_t *r8) // SWAP \w; 2; 8; Z 0 0 0
{
	*r8 = (*r8 << 4) | (*r8 >> 4);
	zf = !*r8;
	nf = 0;
	hf = 0;
	cf = 0;
}

static void
bit8(uint8_t d8, uint8_t bit) // BIT \d,\w; 2; 8; Z 0 1 -
{
	zf = !(d8 & (1 << bit));
	nf = 0;
	hf = 1;
}

static void
set8(uint8_t *r8, uint8_t bit) // SET \d,\w; 2; 8; ----
{
	*r8 |= 1 << bit;
}

static void
sethl(uint8_t bit)
{
	uint8_t d8 = mem_read(hl);
	set8(&d8, bit);
	mem_write(hl, d8);
}

static void
res8(uint8_t *r8, uint8_t bit) // RES \d,\w; 2; 8; ----
{
	*r8 &= ~(1 << bit);
}

static void
reshl(uint8_t bit)
{
	uint8_t d8 = mem_read(hl);
	res8(&d8, bit);
	mem_write(hl, d8);
}


#pragma mark - Init

void
cpu_init()
{
#if 0
	disable_bootrom();
	a = 1;
	bc = 0x13;
	de = 0xd8;
	sp = 0xfffe;
	pc = 0x100;
	zf = 1;
	nf = 0;
	hf = 0;
	cf = 1;
#endif
}


#pragma mark - Steps
int counter = 0;

#define NOT_YET_IMPLEMENTED()	do { \
	printf("todo: pc=0x%04x, opcode=0x%02x\n", pc, opcode); \
	return 1; \
} while(0);


#pragma mark - IRQ

int halted = 0;

int
cpu_step()
{
	uint8_t pending_irqs = io_get_pending_irqs();
	while ((interrupts_enabled || halted) && pending_irqs) {
		interrupts_enabled = 0;
		int i;
		for (i = 0; i < 8; i++) {
			if (pending_irqs & (1 << i)) {
				break;
			}
		}
		if (halted) {
			pc += 2; // DMG bug: skip instruction after HALT
		} else {
			io_clear_pending_irq(i);
		}
		halted = 0;
//		printf("RST 0x%02x\n", 0x40 + i * 8);
		rst8(0x40 + i * 8);
	}

	uint8_t opcode = fetch8();
//			if (++counter % 100 == 0) {
			if (!is_bootrom_enabled()) {
//				printf("A=%02x BC=%04x DE=%04x HL=%04x SP=%04x PC=%04x (ZF=%d,NF=%d,HF=%d,CF=%d) LY=%02x - opcode 0x%02x\n", a, bc, de, hl, sp, pc-1, zf, nf, hf, cf, mem_read(0xff44), opcode);
			}

	switch (opcode) {
		case 0x00: // NOP; 1; 4; ----
			break;
		case 0x01: // LD BC,d16; 3; 12; ----
			bc = fetch16();
			break;
		case 0x02: // LD (BC),A; 1; 8; ----
			mem_write(bc, a);
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
			b = fetch8();
			break;
		case 0x07: { // RLCA; 1; 4; 0 0 0 C
			cf = a >> 7;
			a = a << 1;
			zf = 0;
			nf = 0;
			hf = 0;
			break;
		}
		case 0x08: { // LD (a16),SP; 3; 20; ----
			uint16_t a16 = fetch16();
			mem_write(a16, sp & 0xff);
			mem_write(a16 + 1, sp >> 8);
			break;
		}
		case 0x09: // ADD HL,BC; 1; 8; - 0 H C
			addhl(bc);
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
			c = fetch8();
			break;
		case 0x0f: { // RRCA; 1; 4; 0 0 0 C
			rrc8(&a);
			zf = 0;
			break;
		}
		case 0x10: // STOP 0; 2; 4; ----
			NOT_YET_IMPLEMENTED();
			break;
		case 0x11: // LD DE,d16; 3; 12; ----
			de = fetch16();
			break;
		case 0x12: // LD (DE),A; 1; 8; ----
			mem_write(de, a);
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
			d = fetch8();
			break;
		case 0x17: { // RLA; 1; 4; 0 0 0 C
			uint8_t old_cf = cf;
			cf = a >> 7;
			a = (a << 1) | old_cf;
			zf = 0;
			nf = 0;
			hf = 0;
			break;
		}
		case 0x18: // JR r8; 2; 12; ----
			jrcc(1);
			break;
		case 0x19: // ADD HL,DE; 1; 8; - 0 H C
			addhl(de);
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
			e = fetch8();
			break;
		case 0x1f: { // RRA; 1; 4; 0 0 0 C
			rr8(&a);
			zf = 0;
			break;
		}
		case 0x20: { // JR NZ,r8; 2; 12/8; ----
			jrcc(!zf);
			break;
		}
		case 0x21: // LD HL,d16; 3; 12; ----
			hl = fetch16();
			break;
		case 0x22: // LD (HL+),A; 1; 8; ---- // LD (HLI),A or LDI (HL),A // target, source
			mem_write(hl++, a);
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
			h = fetch8();
			break;
		case 0x27: // DAA; 1; 4; Z - 0 C
			// see: http://z80-heaven.wikidot.com/instructions-set:daa
			NOT_YET_IMPLEMENTED();
			break;
		case 0x28: // JR Z,r8; 2; 12/8; ----
			jrcc(zf);
			break;
		case 0x29: // ADD HL,HL; 1; 8; - 0 H C
			addhl(hl);
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
			l = fetch8();
			break;
		case 0x2f: // CPL; 1; 4; - 1 1 -
			a = ~a;
			nf = 1;
			hf = 1;
			break;
		case 0x30: // JR NC,r8; 2; 12/8; ----
			jrcc(!cf);
			break;
		case 0x31: // LD SP,d16; 3; 12; ----
			sp = fetch16();
			break;
		case 0x32: // LD (HL-),A; 1; 8; ---- // LD (HLD),A or LDD (HL),A // target, source
			mem_write(hl--, a);
			break;
		case 0x33: // INC SP; 1; 8; ----
			sp++;
			break;
		case 0x34: { // INC (HL); 1; 12; Z 0 H -
			uint8_t d8 = mem_read(hl);
			inc8(&d8);
			mem_write(hl, d8);
			break;
		}
		case 0x35: { // DEC (HL); 1; 12; Z 1 H -
			uint8_t d8 = mem_read(hl);
			dec8(&d8);
			mem_write(hl, d8);
			break;
		}
		case 0x36: // LD (HL),d8; 2; 12; ----
			mem_write(hl, fetch8());
			break;
		case 0x37: // SCF; 1; 4; - 0 0 1
			nf = 0;
			hf = 0;
			cf = 1;
			break;
		case 0x38: // JR C,r8; 2; 12/8; ----
			jrcc(cf);
			break;
		case 0x39: // ADD HL,SP; 1; 8; - 0 H C
			addhl(sp);
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
			a = fetch8();
			break;
		case 0x3f: // CCF; 1; 4; - 0 0 C
			nf = 0;
			hf = 0;
			cf = ~cf;
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
			mem_write(hl, b);
			break;
		case 0x71: // LD (HL),C; 1; 8; ----
			mem_write(hl, c);
			break;
		case 0x72: // LD (HL),D; 1; 8; ----
			mem_write(hl, d);
			break;
		case 0x73: // LD (HL),E; 1; 8; ----
			mem_write(hl, e);
			break;
		case 0x74: // LD (HL),H; 1; 8; ----
			mem_write(hl, h);
			break;
		case 0x75: // LD (HL),L; 1; 8; ----
			mem_write(hl, l);
			break;
		case 0x76: // HALT; 1; 4; ----
			halted = 1;
			pc--;
			break;
		case 0x77: // LD (HL),A; 1; 8; ----
			mem_write(hl, a);
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
			adda8(b);
			break;
		case 0x81: // ADD A,C; 1; 4; Z 0 H C
			adda8(c);
			break;
		case 0x82: // ADD A,D; 1; 4; Z 0 H C
			adda8(d);
			break;
		case 0x83: // ADD A,E; 1; 4; Z 0 H C
			adda8(e);
			break;
		case 0x84: // ADD A,H; 1; 4; Z 0 H C
			adda8(h);
			break;
		case 0x85: // ADD A,L; 1; 4; Z 0 H C
			adda8(l);
			break;
		case 0x86: // ADD A,(HL); 1; 8; Z 0 H C
			adda8(mem_read(hl));
			break;
		case 0x87: // ADD A,A; 1; 4; Z 0 H C
			adda8(a);
			break;
		case 0x88: // ADC A,B; 1; 4; Z 0 H C
			adca8(b);
			break;
		case 0x89: // ADC A,C; 1; 4; Z 0 H C
			adca8(c);
			break;
		case 0x8a: // ADC A,D; 1; 4; Z 0 H C
			adca8(d);
			break;
		case 0x8b: // ADC A,E; 1; 4; Z 0 H C
			adca8(e);
			break;
		case 0x8c: // ADC A,H; 1; 4; Z 0 H C
			adca8(h);
			break;
		case 0x8d: // ADC A,L; 1; 4; Z 0 H C
			adca8(l);
			break;
		case 0x8e: // ADC A,(HL); 1; 8; Z 0 H C
			adca8(mem_read(hl));
			break;
		case 0x8f: // ADC A,A; 1; 4; Z 0 H C
			adca8(a);
			break;
		case 0x90: // SUB B; 1; 4; Z 1 H C
			suba8(b);
			break;
		case 0x91: // SUB C; 1; 4; Z 1 H C
			suba8(c);
			break;
		case 0x92: // SUB D; 1; 4; Z 1 H C
			suba8(d);
			break;
		case 0x93: // SUB E; 1; 4; Z 1 H C
			suba8(e);
			break;
		case 0x94: // SUB H; 1; 4; Z 1 H C
			suba8(h);
			break;
		case 0x95: // SUB L; 1; 4; Z 1 H C
			suba8(l);
			break;
		case 0x96: // SUB (HL); 1; 8; Z 1 H C
			suba8(mem_read(hl));
			break;
		case 0x97: // SUB A; 1; 4; Z 1 H C
			suba8(a);
			break;
		case 0x98: // SBC A,B; 1; 4; Z 1 H C
			sbca8(b);
			break;
		case 0x99: // SBC A,C; 1; 4; Z 1 H C
			sbca8(c);
			break;
		case 0x9a: // SBC A,D; 1; 4; Z 1 H C
			sbca8(d);
			break;
		case 0x9b: // SBC A,E; 1; 4; Z 1 H C
			sbca8(e);
			break;
		case 0x9c: // SBC A,H; 1; 4; Z 1 H C
			sbca8(h);
			break;
		case 0x9d: // SBC A,L; 1; 4; Z 1 H C
			sbca8(l);
			break;
		case 0x9e: // SBC A,(HL); 1; 8; Z 1 H C
			sbca8(mem_read(hl));
			break;
		case 0x9f: // SBC A,A; 1; 4; Z 1 H C
			sbca8(a);
			break;
		case 0xa0: // AND B; 1; 4; Z 0 1 0
			anda(b);
			break;
		case 0xa1: // AND C; 1; 4; Z 0 1 0
			anda(c);
			break;
		case 0xa2: // AND D; 1; 4; Z 0 1 0
			anda(d);
			break;
		case 0xa3: // AND E; 1; 4; Z 0 1 0
			anda(e);
			break;
		case 0xa4: // AND H; 1; 4; Z 0 1 0
			anda(h);
			break;
		case 0xa5: // AND L; 1; 4; Z 0 1 0
			anda(l);
			break;
		case 0xa6: // AND (HL); 1; 8; Z 0 1 0
			anda(mem_read(hl));
			break;
		case 0xa7: // AND A; 1; 4; Z 0 1 0
			anda(a);
			break;
		case 0xa8: // XOR B; 1; 4; Z 0 0 0
			xora(b);
			break;
		case 0xa9: // XOR C; 1; 4; Z 0 0 0
			xora(c);
			break;
		case 0xaa: // XOR D; 1; 4; Z 0 0 0
			xora(d);
			break;
		case 0xab: // XOR E; 1; 4; Z 0 0 0
			xora(e);
			break;
		case 0xac: // XOR H; 1; 4; Z 0 0 0
			xora(h);
			break;
		case 0xad: // XOR L; 1; 4; Z 0 0 0
			xora(l);
			break;
		case 0xae: // XOR (HL); 1; 8; Z 0 0 0
			xora(mem_read(hl));
			break;
		case 0xaf: // XOR A; 1; 4; Z 0 0 0
			xora(a);
			break;
		case 0xb0: // OR B; 1; 4; Z 0 0 0
			ora(b);
			break;
		case 0xb1: // OR C; 1; 4; Z 0 0 0
			ora(c);
			break;
		case 0xb2: // OR D; 1; 4; Z 0 0 0
			ora(d);
			break;
		case 0xb3: // OR E; 1; 4; Z 0 0 0
			ora(e);
			break;
		case 0xb4: // OR H; 1; 4; Z 0 0 0
			ora(h);
			break;
		case 0xb5: // OR L; 1; 4; Z 0 0 0
			ora(l);
			break;
		case 0xb6: // OR (HL); 1; 8; Z 0 0 0
			ora(mem_read(hl));
			break;
		case 0xb7: // OR A; 1; 4; Z 0 0 0
			ora(a);
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
			retcc(!zf);
			break;
		case 0xc1: // POP BC; 1; 12; ----
			bc = pop16();
			break;
		case 0xc2: // JP NZ,a16; 3; 16/12; ----
			jpcc(!zf);
			break;
		case 0xc3: { // JP a16; 3; 16; ----
			jpcc(1);
			break;
		}
		case 0xc4: // CALL NZ,a16; 3; 24/12; ----
			callcc(!zf);
			break;
		case 0xc5: // PUSH BC; 1; 16; ----
			push16(bc);
			break;
		case 0xc6: // ADD A,d8; 2; 8; Z 0 H C
			adda8(fetch8());
			break;
		case 0xc7: // RST 00H; 1; 16; ----
			rst8(0x00);
			break;
		case 0xc8: // RET Z; 1; 20/8; ----
			retcc(zf);
			break;
		case 0xc9: // RET; 1; 16; ----
			retcc(1);
			break;
		case 0xca: // JP Z,a16; 3; 16/12; ----
			jpcc(zf);
			break;

		case 0xcb: // PREFIX CB
			opcode = fetch8();
//			printf("0x%02x\n", opcode);

			switch (opcode) {
				case 0x00: // RLC B; 2; 8; Z 0 0 C
					rlc8(&b);
					break;
				case 0x01: // RLC C; 2; 8; Z 0 0 C
					rlc8(&c);
					break;
				case 0x02: // RLC D; 2; 8; Z 0 0 C
					rlc8(&d);
					break;
				case 0x03: // RLC E; 2; 8; Z 0 0 C
					rlc8(&e);
					break;
				case 0x04: // RLC H; 2; 8; Z 0 0 C
					rlc8(&h);
					break;
				case 0x05: // RLC L; 2; 8; Z 0 0 C
					rlc8(&l);
					break;
				case 0x06: { // RLC (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					rlc8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x07: // RLC A; 2; 8; Z 0 0 C
					rlc8(&a);
					break;
				case 0x08: // RRC B; 2; 8; Z 0 0 C
					rrc8(&b);
					break;
				case 0x09: // RRC C; 2; 8; Z 0 0 C
					rrc8(&c);
					break;
				case 0x0a: // RRC D; 2; 8; Z 0 0 C
					rrc8(&d);
					break;
				case 0x0b: // RRC E; 2; 8; Z 0 0 C
					rrc8(&e);
					break;
				case 0x0c: // RRC H; 2; 8; Z 0 0 C
					rrc8(&h);
					break;
				case 0x0d: // RRC L; 2; 8; Z 0 0 C
					rrc8(&l);
					break;
				case 0x0e: { // RRC (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					rrc8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x0f: // RRC A; 2; 8; Z 0 0 C
					rrc8(&a);
					break;
				case 0x10: // RL B; 2; 8; Z 0 0 C
					rl8(&b);
					break;
				case 0x11: // RL C; 2; 8; Z 0 0 C
					rl8(&c);
					break;
				case 0x12: // RL D; 2; 8; Z 0 0 C
					rl8(&d);
					break;
				case 0x13: // RL E; 2; 8; Z 0 0 C
					rl8(&e);
					break;
				case 0x14: // RL H; 2; 8; Z 0 0 C
					rl8(&h);
					break;
				case 0x15: // RL L; 2; 8; Z 0 0 C
					rl8(&l);
					break;
				case 0x16: { // RL (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					rl8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x17: // RL A; 2; 8; Z 0 0 C
					rl8(&a);
					break;
				case 0x18: // RR B; 2; 8; Z 0 0 C
					rr8(&b);
					break;
				case 0x19: // RR C; 2; 8; Z 0 0 C
					rr8(&c);
					break;
				case 0x1a: // RR D; 2; 8; Z 0 0 C
					rr8(&d);
					break;
				case 0x1b: // RR E; 2; 8; Z 0 0 C
					rr8(&e);
					break;
				case 0x1c: // RR H; 2; 8; Z 0 0 C
					rr8(&h);
					break;
				case 0x1d: // RR L; 2; 8; Z 0 0 C
					rr8(&l);
					break;
				case 0x1e: { // RR (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					rr8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x1f: // RR A; 2; 8; Z 0 0 C
					rr8(&a);
					break;
				case 0x20: // SLA B; 2; 8; Z 0 0 C
					sla8(&b);
					break;
				case 0x21: // SLA C; 2; 8; Z 0 0 C
					sla8(&c);
					break;
				case 0x22: // SLA D; 2; 8; Z 0 0 C
					sla8(&d);
					break;
				case 0x23: // SLA E; 2; 8; Z 0 0 C
					sla8(&e);
					break;
				case 0x24: // SLA H; 2; 8; Z 0 0 C
					sla8(&h);
					break;
				case 0x25: // SLA L; 2; 8; Z 0 0 C
					sla8(&l);
					break;
				case 0x26: { // SLA (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					sla8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x27: // SLA A; 2; 8; Z 0 0 C
					sla8(&a);
					break;
				case 0x28: // SRA B; 2; 8; Z 0 0 0
					sra8(&b);
					break;
				case 0x29: // SRA C; 2; 8; Z 0 0 0
					sra8(&c);
					break;
				case 0x2a: // SRA D; 2; 8; Z 0 0 0
					sra8(&d);
					break;
				case 0x2b: // SRA E; 2; 8; Z 0 0 0
					sra8(&e);
					break;
				case 0x2c: // SRA H; 2; 8; Z 0 0 0
					sra8(&h);
					break;
				case 0x2d: // SRA L; 2; 8; Z 0 0 0
					sra8(&l);
					break;
				case 0x2e: { // SRA (HL); 2; 16; Z 0 0 0
					uint8_t d8 = mem_read(hl);
					sra8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x2f: // SRA A; 2; 8; Z 0 0 0
					sra8(&a);
					break;
				case 0x30: // SWAP B; 2; 8; Z 0 0 0
					swap8(&b);
					break;
				case 0x31: // SWAP C; 2; 8; Z 0 0 0
					swap8(&c);
					break;
				case 0x32: // SWAP D; 2; 8; Z 0 0 0
					swap8(&d);
					break;
				case 0x33: // SWAP E; 2; 8; Z 0 0 0
					swap8(&e);
					break;
				case 0x34: // SWAP H; 2; 8; Z 0 0 0
					swap8(&h);
					break;
				case 0x35: // SWAP L; 2; 8; Z 0 0 0
					swap8(&l);
					break;
				case 0x36: { // SWAP (HL); 2; 16; Z 0 0 0
					uint8_t d8 = mem_read(hl);
					swap8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x37: // SWAP A; 2; 8; Z 0 0 0
					swap8(&a);
					break;
				case 0x38: // SRL B; 2; 8; Z 0 0 C
					srl8(&b);
					break;
				case 0x39: // SRL C; 2; 8; Z 0 0 C
					srl8(&c);
					break;
				case 0x3a: // SRL D; 2; 8; Z 0 0 C
					srl8(&d);
					break;
				case 0x3b: // SRL E; 2; 8; Z 0 0 C
					srl8(&e);
					break;
				case 0x3c: // SRL H; 2; 8; Z 0 0 C
					srl8(&h);
					break;
				case 0x3d: // SRL L; 2; 8; Z 0 0 C
					srl8(&l);
					break;
				case 0x3e: { // SRL (HL); 2; 16; Z 0 0 C
					uint8_t d8 = mem_read(hl);
					srl8(&d8);
					mem_write(hl, d8);
					break;
				}
				case 0x3f: // SRL A; 2; 8; Z 0 0 C
					srl8(&a);
					break;
				case 0x40: // BIT 0,B; 2; 8; Z 0 1 -
					bit8(b, 0);
					break;
				case 0x41: // BIT 0,C; 2; 8; Z 0 1 -
					bit8(c, 0);
					break;
				case 0x42: // BIT 0,D; 2; 8; Z 0 1 -
					bit8(d, 0);
					break;
				case 0x43: // BIT 0,E; 2; 8; Z 0 1 -
					bit8(e, 0);
					break;
				case 0x44: // BIT 0,H; 2; 8; Z 0 1 -
					bit8(h, 0);
					break;
				case 0x45: // BIT 0,L; 2; 8; Z 0 1 -
					bit8(l, 0);
					break;
				case 0x46: // BIT 0,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 0);
					break;
				case 0x47: // BIT 0,A; 2; 8; Z 0 1 -
					bit8(a, 0);
					break;
				case 0x48: // BIT 1,B; 2; 8; Z 0 1 -
					bit8(b, 1);
					break;
				case 0x49: // BIT 1,C; 2; 8; Z 0 1 -
					bit8(c, 1);
					break;
				case 0x4a: // BIT 1,D; 2; 8; Z 0 1 -
					bit8(d, 1);
					break;
				case 0x4b: // BIT 1,E; 2; 8; Z 0 1 -
					bit8(e, 1);
					break;
				case 0x4c: // BIT 1,H; 2; 8; Z 0 1 -
					bit8(h, 1);
					break;
				case 0x4d: // BIT 1,L; 2; 8; Z 0 1 -
					bit8(l, 1);
					break;
				case 0x4e: // BIT 1,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 1);
					break;
				case 0x4f: // BIT 1,A; 2; 8; Z 0 1 -
					bit8(a, 1);
					break;
				case 0x50: // BIT 2,B; 2; 8; Z 0 1 -
					bit8(b, 2);
					break;
				case 0x51: // BIT 2,C; 2; 8; Z 0 1 -
					bit8(c, 2);
					break;
				case 0x52: // BIT 2,D; 2; 8; Z 0 1 -
					bit8(d, 2);
					break;
				case 0x53: // BIT 2,E; 2; 8; Z 0 1 -
					bit8(e, 2);
					break;
				case 0x54: // BIT 2,H; 2; 8; Z 0 1 -
					bit8(h, 2);
					break;
				case 0x55: // BIT 2,L; 2; 8; Z 0 1 -
					bit8(l, 2);
					break;
				case 0x56: // BIT 2,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 2);
					break;
				case 0x57: // BIT 2,A; 2; 8; Z 0 1 -
					bit8(a, 2);
					break;
				case 0x58: // BIT 3,B; 2; 8; Z 0 1 -
					bit8(b, 3);
					break;
				case 0x59: // BIT 3,C; 2; 8; Z 0 1 -
					bit8(c, 3);
					break;
				case 0x5a: // BIT 3,D; 2; 8; Z 0 1 -
					bit8(d, 3);
					break;
				case 0x5b: // BIT 3,E; 2; 8; Z 0 1 -
					bit8(e, 3);
					break;
				case 0x5c: // BIT 3,H; 2; 8; Z 0 1 -
					bit8(h, 3);
					break;
				case 0x5d: // BIT 3,L; 2; 8; Z 0 1 -
					bit8(l, 3);
					break;
				case 0x5e: // BIT 3,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 3);
					break;
				case 0x5f: // BIT 3,A; 2; 8; Z 0 1 -
					bit8(a, 3);
					break;
				case 0x60: // BIT 4,B; 2; 8; Z 0 1 -
					bit8(b, 4);
					break;
				case 0x61: // BIT 4,C; 2; 8; Z 0 1 -
					bit8(c, 4);
					break;
				case 0x62: // BIT 4,D; 2; 8; Z 0 1 -
					bit8(d, 4);
					break;
				case 0x63: // BIT 4,E; 2; 8; Z 0 1 -
					bit8(e, 4);
					break;
				case 0x64: // BIT 4,H; 2; 8; Z 0 1 -
					bit8(h, 4);
					break;
				case 0x65: // BIT 4,L; 2; 8; Z 0 1 -
					bit8(l, 4);
					break;
				case 0x66: // BIT 4,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 4);
					break;
				case 0x67: // BIT 4,A; 2; 8; Z 0 1 -
					bit8(a, 4);
					break;
				case 0x68: // BIT 5,B; 2; 8; Z 0 1 -
					bit8(b, 5);
					break;
				case 0x69: // BIT 5,C; 2; 8; Z 0 1 -
					bit8(c, 5);
					break;
				case 0x6a: // BIT 5,D; 2; 8; Z 0 1 -
					bit8(d, 5);
					break;
				case 0x6b: // BIT 5,E; 2; 8; Z 0 1 -
					bit8(e, 5);
					break;
				case 0x6c: // BIT 5,H; 2; 8; Z 0 1 -
					bit8(h, 5);
					break;
				case 0x6d: // BIT 5,L; 2; 8; Z 0 1 -
					bit8(l, 5);
					break;
				case 0x6e: // BIT 5,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 5);
					break;
				case 0x6f: // BIT 5,A; 2; 8; Z 0 1 -
					bit8(a, 5);
					break;
				case 0x70: // BIT 6,B; 2; 8; Z 0 1 -
					bit8(b, 6);
					break;
				case 0x71: // BIT 6,C; 2; 8; Z 0 1 -
					bit8(c, 6);
					break;
				case 0x72: // BIT 6,D; 2; 8; Z 0 1 -
					bit8(d, 6);
					break;
				case 0x73: // BIT 6,E; 2; 8; Z 0 1 -
					bit8(e, 6);
					break;
				case 0x74: // BIT 6,H; 2; 8; Z 0 1 -
					bit8(h, 6);
					break;
				case 0x75: // BIT 6,L; 2; 8; Z 0 1 -
					bit8(l, 6);
					break;
				case 0x76: // BIT 6,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 6);
					break;
				case 0x77: // BIT 6,A; 2; 8; Z 0 1 -
					bit8(a, 6);
					break;
				case 0x78: // BIT 7,B; 2; 8; Z 0 1 -
					bit8(b, 7);
					break;
				case 0x79: // BIT 7,C; 2; 8; Z 0 1 -
					bit8(c, 7);
					break;
				case 0x7a: // BIT 7,D; 2; 8; Z 0 1 -
					bit8(d, 7);
					break;
				case 0x7b: // BIT 7,E; 2; 8; Z 0 1 -
					bit8(e, 7);
					break;
				case 0x7c: // BIT 7,H; 2; 8; Z 0 1 -
					bit8(h, 7);
					break;
				case 0x7d: // BIT 7,L; 2; 8; Z 0 1 -
					bit8(l, 7);
					break;
				case 0x7e: // BIT 7,(HL); 2; 16; Z 0 1 -
					bit8(mem_read(hl), 7);
					break;
				case 0x7f: // BIT 7,A; 2; 8; Z 0 1 -
					bit8(a, 7);
					break;
				case 0x80: // RES 0,B; 2; 8; ----
					res8(&b, 0);
					break;
				case 0x81: // RES 0,C; 2; 8; ----
					res8(&c, 0);
					break;
				case 0x82: // RES 0,D; 2; 8; ----
					res8(&d, 0);
					break;
				case 0x83: // RES 0,E; 2; 8; ----
					res8(&e, 0);
					break;
				case 0x84: // RES 0,H; 2; 8; ----
					res8(&h, 0);
					break;
				case 0x85: // RES 0,L; 2; 8; ----
					res8(&l, 0);
					break;
				case 0x86: // RES 0,(HL); 2; 16; ----
					reshl(0);
					break;
				case 0x87: // RES 0,A; 2; 8; ----
					res8(&a, 0);
					break;
				case 0x88: // RES 1,B; 2; 8; ----
					res8(&b, 1);
					break;
				case 0x89: // RES 1,C; 2; 8; ----
					res8(&c, 1);
					break;
				case 0x8a: // RES 1,D; 2; 8; ----
					res8(&d, 1);
					break;
				case 0x8b: // RES 1,E; 2; 8; ----
					res8(&e, 1);
					break;
				case 0x8c: // RES 1,H; 2; 8; ----
					res8(&h, 1);
					break;
				case 0x8d: // RES 1,L; 2; 8; ----
					res8(&l, 1);
					break;
				case 0x8e: // RES 1,(HL); 2; 16; ----
					reshl(1);
					break;
				case 0x8f: // RES 1,A; 2; 8; ----
					res8(&a, 1);
					break;
				case 0x90: // RES 2,B; 2; 8; ----
					res8(&b, 2);
					break;
				case 0x91: // RES 2,C; 2; 8; ----
					res8(&c, 2);
					break;
				case 0x92: // RES 2,D; 2; 8; ----
					res8(&d, 2);
					break;
				case 0x93: // RES 2,E; 2; 8; ----
					res8(&e, 2);
					break;
				case 0x94: // RES 2,H; 2; 8; ----
					res8(&h, 2);
					break;
				case 0x95: // RES 2,L; 2; 8; ----
					res8(&l, 2);
					break;
				case 0x96: // RES 2,(HL); 2; 16; ----
					reshl(2);
					break;
				case 0x97: // RES 2,A; 2; 8; ----
					res8(&a, 2);
					break;
				case 0x98: // RES 3,B; 2; 8; ----
					res8(&b, 3);
					break;
				case 0x99: // RES 3,C; 2; 8; ----
					res8(&c, 3);
					break;
				case 0x9a: // RES 3,D; 2; 8; ----
					res8(&d, 3);
					break;
				case 0x9b: // RES 3,E; 2; 8; ----
					res8(&e, 3);
					break;
				case 0x9c: // RES 3,H; 2; 8; ----
					res8(&h, 3);
					break;
				case 0x9d: // RES 3,L; 2; 8; ----
					res8(&l, 3);
					break;
				case 0x9e: // RES 3,(HL); 2; 16; ----
					reshl(3);
					break;
				case 0x9f: // RES 3,A; 2; 8; ----
					res8(&a, 3);
					break;
				case 0xa0: // RES 4,B; 2; 8; ----
					res8(&b, 4);
					break;
				case 0xa1: // RES 4,C; 2; 8; ----
					res8(&c, 4);
					break;
				case 0xa2: // RES 4,D; 2; 8; ----
					res8(&d, 4);
					break;
				case 0xa3: // RES 4,E; 2; 8; ----
					res8(&e, 4);
					break;
				case 0xa4: // RES 4,H; 2; 8; ----
					res8(&h, 4);
					break;
				case 0xa5: // RES 4,L; 2; 8; ----
					res8(&l, 4);
					break;
				case 0xa6: // RES 4,(HL); 2; 16; ----
					reshl(4);
					break;
				case 0xa7: // RES 4,A; 2; 8; ----
					res8(&a, 4);
					break;
				case 0xa8: // RES 5,B; 2; 8; ----
					res8(&b, 5);
					break;
				case 0xa9: // RES 5,C; 2; 8; ----
					res8(&c, 5);
					break;
				case 0xaa: // RES 5,D; 2; 8; ----
					res8(&d, 5);
					break;
				case 0xab: // RES 5,E; 2; 8; ----
					res8(&e, 5);
					break;
				case 0xac: // RES 5,H; 2; 8; ----
					res8(&h, 5);
					break;
				case 0xad: // RES 5,L; 2; 8; ----
					res8(&l, 5);
					break;
				case 0xae: // RES 5,(HL); 2; 16; ----
					reshl(5);
					break;
				case 0xaf: // RES 5,A; 2; 8; ----
					res8(&a, 5);
					break;
				case 0xb0: // RES 6,B; 2; 8; ----
					res8(&b, 6);
					break;
				case 0xb1: // RES 6,C; 2; 8; ----
					res8(&c, 6);
					break;
				case 0xb2: // RES 6,D; 2; 8; ----
					res8(&d, 6);
					break;
				case 0xb3: // RES 6,E; 2; 8; ----
					res8(&e, 6);
					break;
				case 0xb4: // RES 6,H; 2; 8; ----
					res8(&h, 6);
					break;
				case 0xb5: // RES 6,L; 2; 8; ----
					res8(&l, 6);
					break;
				case 0xb6: // RES 6,(HL); 2; 16; ----
					reshl(6);
					break;
				case 0xb7: // RES 6,A; 2; 8; ----
					res8(&a, 6);
					break;
				case 0xb8: // RES 7,B; 2; 8; ----
					res8(&b, 7);
					break;
				case 0xb9: // RES 7,C; 2; 8; ----
					res8(&c, 7);
					break;
				case 0xba: // RES 7,D; 2; 8; ----
					res8(&d, 7);
					break;
				case 0xbb: // RES 7,E; 2; 8; ----
					res8(&e, 7);
					break;
				case 0xbc: // RES 7,H; 2; 8; ----
					res8(&h, 7);
					break;
				case 0xbd: // RES 7,L; 2; 8; ----
					res8(&l, 7);
					break;
				case 0xbe: // RES 7,(HL); 2; 16; ----
					reshl(7);
					break;
				case 0xbf: // RES 7,A; 2; 8; ----
					res8(&a, 7);
					break;
				case 0xc0: // SET 0,B; 2; 8; ----
					set8(&b, 0);
					break;
				case 0xc1: // SET 0,C; 2; 8; ----
					set8(&c, 0);
					break;
				case 0xc2: // SET 0,D; 2; 8; ----
					set8(&d, 0);
					break;
				case 0xc3: // SET 0,E; 2; 8; ----
					set8(&e, 0);
					break;
				case 0xc4: // SET 0,H; 2; 8; ----
					set8(&h, 0);
					break;
				case 0xc5: // SET 0,L; 2; 8; ----
					set8(&l, 0);
					break;
				case 0xc6: // SET 0,(HL); 2; 16; ----
					sethl(0);
					break;
				case 0xc7: // SET 0,A; 2; 8; ----
					set8(&a, 0);
					break;
				case 0xc8: // SET 1,B; 2; 8; ----
					set8(&b, 1);
					break;
				case 0xc9: // SET 1,C; 2; 8; ----
					set8(&c, 1);
					break;
				case 0xca: // SET 1,D; 2; 8; ----
					set8(&d, 1);
					break;
				case 0xcb: // SET 1,E; 2; 8; ----
					set8(&e, 1);
					break;
				case 0xcc: // SET 1,H; 2; 8; ----
					set8(&h, 1);
					break;
				case 0xcd: // SET 1,L; 2; 8; ----
					set8(&l, 1);
					break;
				case 0xce: // SET 1,(HL); 2; 16; ----
					sethl(1);
					break;
				case 0xcf: // SET 1,A; 2; 8; ----
					set8(&a, 1);
					break;
				case 0xd0: // SET 2,B; 2; 8; ----
					set8(&b, 2);
					break;
				case 0xd1: // SET 2,C; 2; 8; ----
					set8(&c, 2);
					break;
				case 0xd2: // SET 2,D; 2; 8; ----
					set8(&d, 2);
					break;
				case 0xd3: // SET 2,E; 2; 8; ----
					set8(&e, 2);
					break;
				case 0xd4: // SET 2,H; 2; 8; ----
					set8(&h, 2);
					break;
				case 0xd5: // SET 2,L; 2; 8; ----
					set8(&l, 2);
					break;
				case 0xd6: // SET 2,(HL); 2; 16; ----
					sethl(1);
					break;
				case 0xd7: // SET 2,A; 2; 8; ----
					set8(&a, 2);
					break;
				case 0xd8: // SET 3,B; 2; 8; ----
					set8(&b, 3);
					break;
				case 0xd9: // SET 3,C; 2; 8; ----
					set8(&c, 3);
					break;
				case 0xda: // SET 3,D; 2; 8; ----
					set8(&d, 3);
					break;
				case 0xdb: // SET 3,E; 2; 8; ----
					set8(&e, 3);
					break;
				case 0xdc: // SET 3,H; 2; 8; ----
					set8(&h, 3);
					break;
				case 0xdd: // SET 3,L; 2; 8; ----
					set8(&l, 3);
					break;
				case 0xde: // SET 3,(HL); 2; 16; ----
					sethl(3);
					break;
				case 0xdf: // SET 3,A; 2; 8; ----
					set8(&a, 3);
					break;
				case 0xe0: // SET 4,B; 2; 8; ----
					set8(&b, 4);
					break;
				case 0xe1: // SET 4,C; 2; 8; ----
					set8(&c, 4);
					break;
				case 0xe2: // SET 4,D; 2; 8; ----
					set8(&d, 4);
					break;
				case 0xe3: // SET 4,E; 2; 8; ----
					set8(&e, 4);
					break;
				case 0xe4: // SET 4,H; 2; 8; ----
					set8(&h, 4);
					break;
				case 0xe5: // SET 4,L; 2; 8; ----
					set8(&l, 4);
					break;
				case 0xe6: // SET 4,(HL); 2; 16; ----
					sethl(4);
					break;
				case 0xe7: // SET 4,A; 2; 8; ----
					set8(&a, 4);
					break;
				case 0xe8: // SET 5,B; 2; 8; ----
					set8(&b, 5);
					break;
				case 0xe9: // SET 5,C; 2; 8; ----
					set8(&c, 5);
					break;
				case 0xea: // SET 5,D; 2; 8; ----
					set8(&d, 5);
					break;
				case 0xeb: // SET 5,E; 2; 8; ----
					set8(&e, 5);
					break;
				case 0xec: // SET 5,H; 2; 8; ----
					set8(&h, 5);
					break;
				case 0xed: // SET 5,L; 2; 8; ----
					set8(&l, 5);
					break;
				case 0xee: // SET 5,(HL); 2; 16; ----
					sethl(5);
					break;
				case 0xef: // SET 5,A; 2; 8; ----
					set8(&a, 5);
					break;
				case 0xf0: // SET 6,B; 2; 8; ----
					set8(&b, 6);
					break;
				case 0xf1: // SET 6,C; 2; 8; ----
					set8(&c, 6);
					break;
				case 0xf2: // SET 6,D; 2; 8; ----
					set8(&d, 6);
					break;
				case 0xf3: // SET 6,E; 2; 8; ----
					set8(&e, 6);
					break;
				case 0xf4: // SET 6,H; 2; 8; ----
					set8(&h, 6);
					break;
				case 0xf5: // SET 6,L; 2; 8; ----
					set8(&l, 6);
					break;
				case 0xf6: // SET 6,(HL); 2; 16; ----
					sethl(6);
					break;
				case 0xf7: // SET 6,A; 2; 8; ----
					set8(&a, 6);
					break;
				case 0xf8: // SET 7,B; 2; 8; ----
					set8(&b, 7);
					break;
				case 0xf9: // SET 7,C; 2; 8; ----
					set8(&c, 7);
					break;
				case 0xfa: // SET 7,D; 2; 8; ----
					set8(&d, 7);
					break;
				case 0xfb: // SET 7,E; 2; 8; ----
					set8(&e, 7);
					break;
				case 0xfc: // SET 7,H; 2; 8; ----
					set8(&h, 7);
					break;
				case 0xfd: // SET 7,L; 2; 8; ----
					set8(&l, 7);
					break;
				case 0xfe: // SET 7,(HL); 2; 16; ----
					sethl(7);
					break;
				case 0xff: // SET 7,A; 2; 8; ----
					set8(&a, 7);
					break;

				default:
					printf("Unknown Prefix CB Opcode 0x%02x\n", opcode);
					return 1;
					break;
			}
			break;

		case 0xcc: // CALL Z,a16; 3; 24/12; ----
			callcc(zf);
			break;
		case 0xcd: { // CALL a16; 3; 24; ----
			callcc(1);
			break;
		}
		case 0xce: // ADC A,d8; 2; 8; Z 0 H C
			adca8(fetch8());
			break;
		case 0xcf: // RST 08H; 1; 16; ----
			rst8(0x08);
			break;
		case 0xd0: // RET NC; 1; 20/8; ----
			retcc(!cf);
			break;
		case 0xd1: // POP DE; 1; 12; ----
			de = pop16();
			break;
		case 0xd2: // JP NC,a16; 3; 16/12; ----
			jpcc(!cf);
			break;
		case 0xd3: // crash
			printf("crash: 0x%02x\n", opcode);
			return 1;
		case 0xd4: // CALL NC,a16; 3; 24/12; ----
			callcc(!cf);
			break;
		case 0xd5: // PUSH DE; 1; 16; ----
			push16(de);
			break;
		case 0xd6: // SUB d8; 2; 8; Z 1 H C
			suba8(fetch8());
			break;
		case 0xd7: // RST 10H; 1; 16; ----
			rst8(0x10);
			break;
		case 0xd8: // RET C; 1; 20/8; ----
			retcc(cf);
			break;
		case 0xd9: // RETI; 1; 16; ----
			interrupts_enabled = 1;
			retcc(1);
			break;
		case 0xda: // JP C,a16; 3; 16/12; ----
			jpcc(cf);
			break;
		case 0xdb: // crash
			printf("crash: 0x%02x\n", opcode);
			return 1;
		case 0xdc: // CALL C,a16; 3; 24/12; ----
			callcc(cf);
			break;
		case 0xdd: // crash
			printf("crash: 0x%02x\n", opcode);
			return 1;
		case 0xde: // SBC A,d8; 2; 8; Z 1 H C
			sbca8(fetch8());
			break;
		case 0xdf: // RST 18H; 1; 16; ----
			rst8(0x18);
			break;
		case 0xe0: { // LDH (a8),A; 2; 12; ---- // LD ($FF00+a8),A
			mem_write(0xff00 + fetch8(), a);
			break;
		}
		case 0xe1: // POP HL; 1; 12; ----
			hl = pop16();
			break;
		case 0xe2: // LD (C),A; 1; 8; ---- // LD ($FF00+C),A // target, source
			mem_write(0xff00 + c, a);
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
			anda(fetch8());
			break;
		case 0xe7: // RST 20H; 1; 16; ----
			rst8(0x20);
			break;
		case 0xe8: // ADD SP,r8; 2; 16; 0 0 H C
			addsp(fetch8());
			break;
		case 0xe9: // JP (HL); 1; 4; ----
			pc = hl;
			break;
		case 0xea: // LD (a16),A; 3; 16; ----
			mem_write(fetch16(), a);
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
			xora(fetch8());
			break;
		case 0xef: // RST 28H; 1; 16; ----
			rst8(0x28);
			break;
		case 0xf0: // LDH A,(a8); 2; 12; ---- // LD A,($FF00+a8)
			a = mem_read(0xff00 + fetch8());
			break;
		case 0xf1: // POP AF; 1; 12; Z N H C
			af = pop16();
			reg16_af.low.bit3_0 = 0;
			break;
		case 0xf2: // LD A,(C); 1; 8; ---- // LD A,($FF00+C)
			a = mem_read(0xff00 + c);
			break;
		case 0xf3: // DI; 1; 4; ----
			interrupts_enabled = 0;
			break;
		case 0xf4: // crash
			printf("crash: 0x%02x\n", opcode);
			return 1;
		case 0xf5: // PUSH AF; 1; 16; ----
			push16(af);
			break;
		case 0xf6: // OR d8; 2; 8; Z 0 0 0
			ora(fetch8());
			break;
		case 0xf7: // RST 30H; 1; 16; ----
			rst8(0x30);
			break;
		case 0xf8: // LD HL,SP+r8; 2; 12; 0 0 H C // LDHL SP,r8
			ldhlsp(fetch8());
			break;
		case 0xf9: // LD SP,HL; 1; 8; ----
			sp = hl;
			break;
		case 0xfa: // LD A,(a16); 3; 16; ----
			a = mem_read(fetch16());
			break;
		case 0xfb: // EI; 1; 4; ----
			interrupts_enabled = 1;
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
			rst8(0x38);
			break;

		default:
			printf("Unknown Opcode 0x%02x\n", opcode);
			return 1;
	}

	return 0;
}

int
cpu_ie()
{
	return interrupts_enabled;
}