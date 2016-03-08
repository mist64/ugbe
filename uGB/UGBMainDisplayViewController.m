//
//  ViewController.m
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
//  Copyright © 2016 Michael Steil. All rights reserved.
//

#import "UGBMainDisplayViewController.h"
#import "UGBRomDocument.h"

@implementation UGBMainDisplayViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.wantsLayer = YES;
    self.view.layer.magnificationFilter = kCAFilterNearest;
}

@end
