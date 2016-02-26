//
//  ppu_private.h
//  gbppu
//
//  Created by Michael Steil on 2016-02-26.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#ifndef ppu_private_h
#define ppu_private_h

#define PPU_NUM_LINES 154
#define PPU_CLOCKS_PER_LINE (114*4)
#define PPU_LAST_VISIBLE_LINE 143

#define rP1 0x00
#define rLCDC 0x40
#define rSTAT 0x41
#define rSCY  0x42
#define rSCX  0x43
#define rLY 0x44
#define rLYC  0x45
#define rDMA  0x46
#define rBGP  0x47
#define rOBP0 0x48
#define rOBP1 0x49
#define rSB 0x01
#define rSC 0x02
#define rDIV 0x04
#define rTIMA 0x05
#define rTMA 0x06
#define rTAC 0x07
#define rIF 0x0F
#define rIE 0xFF
#define rWY 0x4A
#define rWX 0x4B
#define rNR50 0x24
#define rNR51 0x25
#define rNR52 0x26
#define rNR10 0x10
#define rNR11 0x11
#define rNR12 0x12
#define rNR13 0x13
#define rNR14 0x14
#define rNR21 0x16
#define rNR22 0x17
#define rNR23 0x18
#define rNR24 0x19
#define rNR30 0x1A
#define rNR31 0x1B
#define rNR32 0x1C
#define rNR33 0x1D
#define rNR34 0x1E
#define rNR41 0x20
#define rNR42 0x21
#define rNR42_2 0x22
#define rNR43 0x23

#define LCDCF_ON      (1 << 7) /* LCD Control Operation */
#define LCDCF_WIN9C00 (1 << 6) /* Window Tile Map Display Select */
#define LCDCF_WINON   (1 << 5) /* Window Display */
#define LCDCF_BG8000  (1 << 4) /* BG & Window Tile Data Select */
#define LCDCF_BG9C00  (1 << 3) /* BG Tile Map Display Select */
#define LCDCF_OBJ16   (1 << 2) /* OBJ Construction */
#define LCDCF_OBJON   (1 << 1) /* OBJ Display */
#define LCDCF_BGON    (1 << 0) /* BG Display */

#endif /* ppu_private_h */