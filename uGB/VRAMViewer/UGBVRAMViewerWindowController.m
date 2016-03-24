//
//  UGBVRAMViewerWindowController.m
//  gbppu
//
//  Created by Dominik Wagner on 3/23/16.
//
//

#import "UGBVRAMViewerWindowController.h"
#import "UGBRomDocument.h"

@interface UGBVRAMViewerWindowController ()

@end

@implementation UGBVRAMViewerWindowController

- (UGBRomDocument *)romDocument {
    return self.document;
}


- (void)windowDidLoad {
    [super windowDidLoad];
    
    self.tilemapView.layer.magnificationFilter = kCAFilterNearest;
}

- (void)showWindow:(id)sender {
    [self window]; // make sure window is loaded
    [self updateTileMapViewContents];
    [super showWindow:sender];
}

- (void)updateTileMapViewContents {
    self.tilemapView.layer.contents = CFBridgingRelease([self.romDocument createImageRefOfTileMap]);
}

- (IBAction)showVRAMViewer:(id)sender {
    [self updateTileMapViewContents];
}

- (void)copy:(id)sender {
    NSPasteboard *board = [NSPasteboard generalPasteboard];
    [board clearContents];
    NSImage *image = [[NSImage alloc] initWithCGImage:(__bridge CGImageRef)self.tilemapView.layer.contents size:NSMakeSize(8*16,256*128/16*8)];
    [board writeObjects:@[image]];
}

@end
