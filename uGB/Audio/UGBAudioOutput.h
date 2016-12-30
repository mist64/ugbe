//
//  UGBAudioOutput.h
//  gbppu
//
//  Created by Dominik Wagner on 12/30/16.
//
//

#import <Foundation/Foundation.h>
#import "TPCircularBuffer.h"

@interface UGBAudioOutput : NSObject 
@property(nonatomic) CGFloat volume;
- (void)startAudio;
- (void)stopAudio;
- (void)pauseAudio;
- (void)resumeAudio;
- (TPCircularBuffer *)inputBuffer;
@end
