//
//  UGBMainWindowController.m
//  uGB
//
//  Created by Dominik Wagner on 3/12/16.
//

#import "UGBMainWindowController.h"

@implementation UGBMainWindowController

- (IBAction)viewScaleItemAction:(NSMenuItem *)aMenuItem {
    CGFloat targetScale = aMenuItem.tag / 10.0;
    CGSize baseSize = (CGSize){.width = 160, .height = 144};
    NSWindow *window = self.window;
    NSRect oldFrame = window.frame;
    CGFloat oldMaxY = CGRectGetMaxY(oldFrame);
    oldFrame = [window contentRectForFrameRect:oldFrame];
    NSRect newFrame = oldFrame;
    newFrame.size = (NSSize){.width  = baseSize.width  * targetScale,
                             .height = baseSize.height * targetScale};
    newFrame = [window frameRectForContentRect:newFrame];
    newFrame.origin.y = oldMaxY - CGRectGetHeight(newFrame);
    [window setFrame:newFrame display:YES animate:YES];
}

@end
