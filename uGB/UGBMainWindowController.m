//
//  UGBMainWindowController.m
//  uGB
//
//  Created by Dominik Wagner on 3/12/16.
//

#import "UGBMainWindowController.h"
#import "UGBRomDocument.h"

@implementation UGBMainWindowController

- (UGBRomDocument *)romDocument {
    return self.document;
}

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

- (IBAction)spaceBarHit:(id)sender {
    self.romDocument.paused = !self.romDocument.isPaused;
    [self synchronizeWindowTitleWithDocumentName];
}

- (IBAction)arrowRightHit:(id)sender {
    if (self.romDocument.isPaused) {
        // single step on pause - should work because of use of semaphore
        self.romDocument.paused = NO;
        self.romDocument.paused = YES;
    }
    [self synchronizeWindowTitleWithDocumentName];
}

- (IBAction)toggleTurbo:(id)sender {
    self.romDocument.turbo = !self.romDocument.turbo;
    [self synchronizeWindowTitleWithDocumentName];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem.action == @selector(toggleTurbo:)) {
        menuItem.state = self.romDocument.turbo ? NSOnState : NSOffState;
        return YES;
    }
    return YES;
}

- (NSString *)windowTitleForDocumentDisplayName:(NSString *)displayName {
    UGBRomDocument *document = self.romDocument;
    if (document.turbo) {
        displayName = [displayName stringByAppendingString:@" ▶︎▶︎"];
    }
    
    if (document.paused) {
        return [NSString stringWithFormat:@"%@ \u275A\u275A (%ld)", displayName, document.frameCount];
    } else {
        return displayName;
    }
}

@end
