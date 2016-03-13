//
//  UGBRomDocument.h
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//

#import <Cocoa/Cocoa.h>
#import "UGBMainDisplayViewController.h"

typedef NS_OPTIONS(uint8_t, UGBKeyCode) {
    UGBKeyCodeRight = 1<<0,
    UGBKeyCodeLeft  = 1<<1,
    UGBKeyCodeUp    = 1<<2,
    UGBKeyCodeDown  = 1<<3,
    UGBKeyCodeA     = 1<<4,
    UGBKeyCodeB     = 1<<5,
    UGBKeyCodeSelect= 1<<6,
    UGBKeyCodeStart = 1<<7,
};

@interface UGBRomDocument : NSDocument

@property (nonatomic, strong) UGBMainDisplayViewController *mainDisplayViewController;
@property (nonatomic) uint8_t keys;

@property (nonatomic, getter=isPaused) BOOL paused;
@property (nonatomic) BOOL turbo;
@property (nonatomic) NSInteger frameCount;

@end

