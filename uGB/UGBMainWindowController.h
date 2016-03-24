//
//  UGBMainWindowController.h
//  uGB
//
//  Created by Dominik Wagner on 3/12/16.
//

#import <Cocoa/Cocoa.h>

@interface UGBMainWindowController : NSWindowController
- (IBAction)spaceBarHit:(id)aSender;
- (IBAction)arrowRightHit:(id)aSender;
- (IBAction)toggleTurbo:(id)sender;

- (IBAction)showVRAMViewer:(id)sender;
@end
