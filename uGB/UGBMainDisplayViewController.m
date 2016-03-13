//
//  ViewController.m
//  uGB
//
//  Created by Dominik Wagner on 3/6/16.
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
