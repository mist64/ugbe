//
//  cpu.h
//  gbppu
//
//  Created by Lisa Brodner on 2016-02-29.
//  Copyright © 2016 Lisa Brodner. All rights reserved.
//

#ifndef cpu_h
#define cpu_h

#include <stdint.h>
#include <stdio.h>

class memory;
class io;

class cpu {
private:
	memory &_memory;
	io     &_io;

private:
	uint16_t pc;
	uint16_t sp;

	int interrupts_enabled;

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

#pragma pack(push, 1)
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
				} bits;
			} low;
			uint8_t high;
		} bytes;
	};
#pragma pack(pop)

	union reg16_t reg16_af;
	union reg16_t reg16_bc;
	union reg16_t reg16_de;
	union reg16_t reg16_hl;

	int counter;
	int halted;

private:
	void internal_delay();
	void set_hf_nf(uint8_t bit, uint16_t da, uint16_t db, uint8_t neg);
	void set_cf(uint8_t bit, uint16_t da, uint16_t db, uint8_t substraction);
	uint8_t fetch8();
	uint16_t fetch16();
	void ldhlsp(uint8_t d8);
	void push8(uint8_t d8);
	void push16(uint16_t d16);
	uint8_t pop8();
	uint16_t pop16();
	void inc8(uint8_t *r8);
	void dec8(uint8_t *r8);
	void comparea8(uint8_t d8, int store);
	void cpa8(uint8_t d8);
	void suba8(uint8_t d8);
	void sbca8(uint8_t d8);
	void adda8(uint8_t d8);
	void adca8(uint8_t d8);
	void addhl(uint16_t d16);
	void addsp(uint8_t d8);
	void jrcc(int condition);
	void jpcc(int condition);
	void rst8(uint8_t d8);
	void retcc(int condition);
	void callcc(int condition);
	void anda(uint8_t d8);
	void ora(uint8_t d8);
	void xora(uint8_t d8);
	void rl8(uint8_t *r8);
	void rlc8(uint8_t *r8);
	void rr8(uint8_t *r8);
	void rrc8(uint8_t *r8);
	void sla8(uint8_t *r8);
	void sra8(uint8_t *r8);
	void srl8(uint8_t *r8);
	void swap8(uint8_t *r8);
	void bit8(uint8_t d8, uint8_t bit);
	void set8(uint8_t *r8, uint8_t bit);
	void sethl(uint8_t bit);
	void res8(uint8_t *r8, uint8_t bit);
	void reshl(uint8_t bit);
	void daa();

protected:
    friend class gb;
	cpu(memory &memory, io &io);

public:
	int step();
};

#endif /* cpu_h */
