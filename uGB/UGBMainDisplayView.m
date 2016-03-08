//
//  UGBMainDisplayView.m
//  gbppu
//
//  Created by Dominik Wagner on 3/8/16.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import "UGBMainDisplayView.h"
#import "UGBRomDocument.h"

@implementation UGBMainDisplayView

- (uint8_t)keyMaskFromEvent:(NSEvent *)event
{
    switch ([event.characters characterAtIndex:0]) {
        case 0xF703: // right
            return 1;
        case 0xF702: // left
            return 2;
        case 0xF700: // up
            return 4;
        case 0xF701: // down
            return 8;
        case 'a': // a
            return 16;
        case 's': // b
            return 32;
        case '\'': // select
            return 64;
        case '\r': // start
            return 128;
        default:
            return 0;
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    ((UGBRomDocument *)(self.window.windowController.document)).keys |= [self keyMaskFromEvent:event];
}

- (void)keyUp:(NSEvent *)event {
    ((UGBRomDocument *)(self.window.windowController.document)).keys &= ~[self keyMaskFromEvent:event];
}

@end
