//
//  UGBAudioOutput.m
//  gbppu
//
//  Created by Dominik Wagner on 12/30/16.
//
//

@import CoreAudio;
@import AudioUnit;
@import AudioToolbox;
#import "UGBAudioOutput.h"


static OSStatus RenderCallback(void                       *circularBuffer,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp       *inTimeStamp,
                               UInt32                      inBusNumber,
                               UInt32                      inNumberFrames,
                               AudioBufferList            *ioData)
{
    int32_t bytesRequested = inNumberFrames * 2 * 2;
    char *outBuffer = ioData->mBuffers[0].mData;
    int availableBytes;
    void *head = TPCircularBufferTail(circularBuffer, &availableBytes);
    int32_t byteCountToWriteOut = MIN(availableBytes, bytesRequested);
    if (byteCountToWriteOut > 0) {
        NSLog(@"Render callback: frames requested: %d - bytes to write out: %d", inNumberFrames, byteCountToWriteOut);
        memcpy(outBuffer, head, byteCountToWriteOut);
        TPCircularBufferConsume(circularBuffer, byteCountToWriteOut);
    } else {
        NSLog(@"Render callback: frames requested: %d - no data available, sending zeros", inNumberFrames);
        memset(outBuffer, 0, bytesRequested);
    }
    
    return noErr;
}


@implementation UGBAudioOutput {
    AUGraph   _graph;
    AUNode    _converterNode, _mixerNode, _outputNode;
    AudioUnit _converterUnit, _mixerUnit, _outputUnit;
    TPCircularBuffer circularBuffer;
}

- (void)cleanupGraphIfNeeded {
    if (_graph) {
        AUGraphStop(_graph);
        AUGraphClose(_graph);
        AUGraphUninitialize(_graph);
    }
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _volume = 1.0;
        TPCircularBufferInit(&circularBuffer, 2 * 2 * 4096);
    }
    return self;
}

- (TPCircularBuffer *)inputBuffer {
    return &circularBuffer;
}

- (void)dealloc {
    [self cleanupGraphIfNeeded];
}

- (void)startAudio {
    [self createGraph];
}

- (void)stopAudio {
    [self cleanupGraphIfNeeded];
}

- (void)pauseAudio {
    AUGraphStop(_graph);
}

- (void)resumeAudio {
    AUGraphStart(_graph);
}

- (void)createGraph {
    OSStatus err;
    
    [self cleanupGraphIfNeeded];
    
    err = NewAUGraph(&_graph);
    if (err) {
        NSLog(@"NewAUGraph failed (%d)", err);
    }
    
    err = AUGraphOpen(_graph);
    if (err) {
        NSLog(@"could not open audio graph (%d)", err);
    }
    
    ComponentDescription desc;
    
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlagsMask    = 0;
    desc.componentFlags        = 0;
    
    // Output node
    err = AUGraphAddNode(_graph, (const AudioComponentDescription *)&desc, &_outputNode);
    if (err) {
        NSLog(@"couldn't create node for output unit (%d)", err);
    }
    
    err = AUGraphNodeInfo(_graph, _outputNode, NULL, &_outputUnit);
    if (err) {
        NSLog(@"couldn't get output from node (%d)", err);
    }
    
    
    desc.componentType = kAudioUnitType_Mixer;
    desc.componentSubType = kAudioUnitSubType_StereoMixer;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    // Mixer node
    err = AUGraphAddNode(_graph, (const AudioComponentDescription *)&desc, &_mixerNode);
    if (err) {
        NSLog(@"couldn't create node for file player (%d)", err);
    }
    
    err = AUGraphNodeInfo(_graph, _mixerNode, NULL, &_mixerUnit);
    if (err) {
        NSLog(@"couldn't get player unit from node (%d)", err);
    }
    
    desc.componentType = kAudioUnitType_FormatConverter;
    desc.componentSubType = kAudioUnitSubType_AUConverter;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    //Create the converter node
    err = AUGraphAddNode(_graph, (const AudioComponentDescription *)&desc, &_converterNode);
    if (err) {
        NSLog(@"couldn't create node for converter (%d)", err);
    }
    
    err = AUGraphNodeInfo(_graph, _converterNode, NULL, &_converterUnit);
    if (err) {
        NSLog(@"couldn't get player unit from converter (%d)", err);
    }
    
    AURenderCallbackStruct renderStruct;
    renderStruct.inputProc = RenderCallback;
    renderStruct.inputProcRefCon = (void *)&circularBuffer; // TODO: give context for the render callback
    
    err = AudioUnitSetProperty(_converterUnit, kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input, 0, &renderStruct, sizeof(AURenderCallbackStruct));
    if (err) {
        NSLog(@"Could not set the render callback (%d)", err);
    }
    
    AudioStreamBasicDescription mDataFormat;
    UInt32 channelCount = 2;
    UInt32 bytesPerSample = 2;
    Float64 sampleRate = 131072; //44100;
    int formatFlag = (bytesPerSample == 4) ? kLinearPCMFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger;
    mDataFormat.mSampleRate       = sampleRate;
    mDataFormat.mFormatID         = kAudioFormatLinearPCM;
    mDataFormat.mFormatFlags      = formatFlag | kAudioFormatFlagsNativeEndian;
    mDataFormat.mBytesPerPacket   = bytesPerSample * channelCount;
    mDataFormat.mFramesPerPacket  = 1; // this means each packet in the AQ has two samples, one for each channel -> 4 bytes/frame/packet
    mDataFormat.mBytesPerFrame    = bytesPerSample * channelCount;
    mDataFormat.mChannelsPerFrame = channelCount;
    mDataFormat.mBitsPerChannel   = 8 * bytesPerSample;
    
    err = AudioUnitSetProperty(_converterUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &mDataFormat, sizeof(AudioStreamBasicDescription));
    if (err) {
        NSLog(@"couldn't set player's input stream format (%d)", err);
    }
    
    err = AUGraphConnectNodeInput(_graph, _converterNode, 0, _mixerNode, 0);
    if (err) {
        NSLog(@"Couldn't connect the converter to the mixer (%d)", err);
    }
    
    
    
    // connect the player to the output unit (stream format will propagate)
    err = AUGraphConnectNodeInput(_graph, _mixerNode, 0, _outputNode, 0);
    if (err) {
        NSLog(@"Could not connect the input of the output (%d)", err);
    }
    
    AudioUnitSetParameter(_outputUnit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Global, 0, 1.0 ,0);
    
    AudioDeviceID outputDeviceID = 0;
    if (outputDeviceID != 0) {
        AudioUnitSetProperty(_outputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &outputDeviceID, sizeof(outputDeviceID));
    }
    
    err = AUGraphInitialize(_graph);
    if (err) {
        NSLog(@"couldn't initialize graph (%d)", err);
    }
    
    err = AUGraphStart(_graph);
    if (err) {
        NSLog(@"couldn't start graph (%d)", err);
    }
    
    CAShow(_graph);
    [self setVolume:[self volume]];
}

- (void)setVolume:(CGFloat)volume
{
    _volume = volume;
    if (_outputUnit) {
        AudioUnitSetParameter(_outputUnit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Global, 0, _volume, 0);
    }
}


@end
