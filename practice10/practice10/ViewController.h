//
//  ViewController.h
//  practice10
//
//  Created by Joel Whang on 6/12/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (nonatomic, weak) IBOutlet UIImageView *image;

- (IBAction)touchBotton:(id)sender;


- (IBAction)incrementer:(UIStepper *)sender;

- (IBAction)refocusViewer:(id)sender;

- (IBAction)angleStepper:(id)sender;


@property (weak, nonatomic) IBOutlet UILabel *regDisplay;

@property (weak, nonatomic) IBOutlet UILabel *timeElapsed;

@property (weak, nonatomic) IBOutlet UIProgressView *progressBar;

@property (weak, nonatomic) IBOutlet UILabel *maxAngle;

@end
