//
//  UGBMainDisplayView.m
//  uGB
//
//  Created by Dominik Wagner on 3/8/16.
//

#import "UGBMainDisplayView.h"
#import "UGBRomDocument.h"
#import "UGBMainWindowController.h"

@implementation UGBMainDisplayView

- (uint8_t)keyMaskFromEvent:(NSEvent *)event
{
    switch ([event.characters characterAtIndex:0]) {
        case 0xF703: // right
            return UGBKeyCodeRight;
        case 0xF702: // left
            return UGBKeyCodeLeft;
        case 0xF700: // up
            return UGBKeyCodeUp;
        case 0xF701: // down
            return UGBKeyCodeDown;
        case 'a': // a
            return UGBKeyCodeA;
        case 's': // b
            return UGBKeyCodeB;
        case '\'': // select
            return UGBKeyCodeSelect;
        case '\r': // start
            return UGBKeyCodeStart;
        default:
            return 0;
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (UGBRomDocument *)romDocument {
    return (UGBRomDocument *)(self.window.windowController.document);
}

- (void)keyDown:(NSEvent *)event {
    switch (event.keyCode) {
        case 49: // spacebar
            [NSApp sendAction:@selector(spaceBarHit:) to:nil from:self];
            break;
        case 0x7c: // right arrow
            [NSApp sendAction:@selector(arrowRightHit:) to:nil from:self];
            break;
        case 0x11: // t for turbo
            [NSApp sendAction:@selector(toggleTurbo:) to:nil from:self];
            break;
        default:
            ;
             NSLog(@"keycode: %x", event.keyCode);
    }
    if (!self.romDocument.isPaused) {
        self.romDocument.keys |= [self keyMaskFromEvent:event];
    }
}

- (void)keyUp:(NSEvent *)event {
    if (!self.romDocument.isPaused) {
        self.romDocument.keys &= ~[self keyMaskFromEvent:event];
    }
}

@end
