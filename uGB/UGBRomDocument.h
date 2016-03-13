//
//  UGBRomDocument.h
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//

#import <Cocoa/Cocoa.h>
#import "UGBMainDisplayViewController.h"

@interface UGBRomDocument : NSDocument

@property (nonatomic, strong) UGBMainDisplayViewController *mainDisplayViewController;
@property (nonatomic) uint8_t keys;

@property (nonatomic, getter=isPaused) BOOL paused;
@property (nonatomic) NSInteger frameCount;

@end

