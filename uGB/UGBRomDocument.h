//
//  Document.h
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "UGBMainDisplayViewController.h"

@interface UGBRomDocument : NSDocument

@property (nonatomic, strong) UGBMainDisplayViewController *mainDisplayViewController;
@property (nonatomic) uint8_t keys;

@end

