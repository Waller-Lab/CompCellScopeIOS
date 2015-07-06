//
//  ViewController.m
//  practice10
//
//  Created by Joel Whang on 6/12/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#import <iostream>
#import "ViewController.h"
#import "qDPC.h"
#import "refocusing.h"
#import "opencv2/imgcodecs/ios.h"
#import "opencv2/highgui/highgui_c.h"
#import "CoreGraphics/CGImage.h"


@interface ViewController ()

@end

@implementation ViewController

double regVal;

int curMaxAngle;

std::vector<cv::Mat> refocusedOutput;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    _image.contentMode = UIViewContentModeScaleAspectFit;
    regVal = (float) .8;
    [_regDisplay setText:[NSString stringWithFormat:@"reg: %f", (float) regVal]];
    curMaxAngle = 10;
    [_maxAngle setText: [NSString stringWithFormat:@"max angle: %d", curMaxAngle]];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (cv::Mat)qDPC:(cv::Mat)left right:(cv::Mat)right top:(cv::Mat)top bottom:(cv::Mat)bottom transfer1:(cv::Mat)transfer1 transfer2:(cv::Mat)transfer2 reg:(double) reg {
    return dpc(left, right, top, bottom, transfer1, transfer2, reg);
}

- (std::vector<cv::Mat>)refocus: (std::vector<std::string> *)stringVector imageVector: (std::vector<CGImageRef> *)imageVector maxAngle:(double)maxAngle{
    return refocus(stringVector, imageVector, maxAngle);
}

- (cv::Mat)CGImageRef2Mat: (CGImageRef)image {
    return CGImageRef2Mat(image);
}

- (IBAction)touchBotton:(id)sender {
    refocusedOutput.clear();
    
    CFTimeInterval startTime = CACurrentMediaTime();
    
    UIImage *left = [UIImage imageNamed:@"dl.jpeg"];
    UIImage *right = [UIImage imageNamed:@"dr.jpeg"];
    UIImage *top = [UIImage imageNamed:@"dt.jpeg"];
    UIImage *bot = [UIImage imageNamed:@"db.jpeg"];
    UIImage *t1 = [UIImage imageNamed:@"dpcTransferFunc_45.tiff"];
    UIImage *t2 = [UIImage imageNamed:@"dpcTransferFunc_135.tiff"];
    UIImage *output;
    
    cv::Mat lMat;
    cv::Mat rMat;
    cv::Mat tMat;
    cv::Mat bMat;
    cv::Mat t1Mat;
    cv::Mat t2Mat;
    cv::Mat outputMat;
    
    UIImageToMat(top, tMat);
    UIImageToMat(bot, bMat);
    UIImageToMat(left, lMat);
    UIImageToMat(right, rMat);
    UIImageToMat(t1, t1Mat);
    UIImageToMat(t2, t2Mat);
    
    CFTimeInterval fooStart = CACurrentMediaTime();
    outputMat = dpc(tMat, bMat, lMat, rMat, t1Mat, t2Mat, regVal);
    CFTimeInterval fooElapsed = CACurrentMediaTime() - fooStart;
    
    output = MatToUIImage(outputMat);
    [_image setImage: output];
    
    CFTimeInterval elapsedTime = CACurrentMediaTime() - startTime;
    
    [_timeElapsed setText: [NSString stringWithFormat:@"time elapsed: %f\n foo elapsed: %f", (float) elapsedTime, (float) fooElapsed]];
}


- (IBAction)refocusButton:(id)sender {
    CFTimeInterval startTime = CACurrentMediaTime();
    std::vector<CGImageRef> imageVector;
    std::vector<std::string> stringVector;
    std::vector<cv::Mat> results;
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSArray *jpegs = [mainBundle pathsForResourcesOfType:@".jpeg" inDirectory:@"sample_data"];
    
    for (int i = 0; i < [jpegs count]; i++) {
        NSString *jpegNSString = (NSString *)[jpegs objectAtIndex:i];
        std::string jpegString([jpegNSString UTF8String]);
        UIImage *img = [UIImage imageNamed: jpegNSString];
        stringVector.insert(stringVector.begin() + i, jpegString);
        imageVector.insert(imageVector.begin() + i, img.CGImage);
    }

    CFTimeInterval fooStart = CACurrentMediaTime();

    results = refocus(&stringVector, &imageVector, curMaxAngle);
    CFTimeInterval fooElapsed = CACurrentMediaTime() - fooStart;
    
    
    std::cout << results.size() << std::endl;
    
    refocusedOutput = results;
    
    imageVector.clear();
    stringVector.clear();
    
    _image.contentMode = UIViewContentModeScaleAspectFit;
    [_image setImage:MatToUIImage(refocusedOutput.at(0))];

    CFTimeInterval elapsedTime = CACurrentMediaTime() - startTime;
    [_timeElapsed setText: [NSString stringWithFormat:@"time elapsed: %f\n foo elapsed: %f", (float) elapsedTime, (float) fooElapsed]];
    
    std::vector<cv::Mat> resultsVector2;


    refocusedOutput.clear();
    
    for (int i = 0; i < results.size()/3; i ++){
        refocusedOutput.insert(refocusedOutput.begin()+i, results.at(i*3));
    }
    for (int i = 0; i < results.size()/3; i ++){
        refocusedOutput.insert(refocusedOutput.begin()+i+results.size()/3, results.at(i*3+1));
    }
    for (int i = 0; i < results.size()/3; i ++){
        refocusedOutput.insert(refocusedOutput.begin()+i+2*results.size()/3, results.at(i*3+2));
    }
//    refocusedOutput = resultsVector2;
    
//    results.clear();
}

- (IBAction)incrementer:(UIStepper *)sender {
    double value = [sender value];
    [_regDisplay setText:[NSString stringWithFormat:@"reg: %f", (float) value]];
    regVal = value;
}

- (IBAction)refocusViewer:(UIStepper *)sender {
    std::cout<<refocusedOutput.size()<<std::endl;
    if (refocusedOutput.size()!=0){
        int value = [sender value];
        std::cout << refocusedOutput.size() << std::endl;
        std::cout << value%refocusedOutput.size() << std::endl;
        UIImage *output = MatToUIImage(refocusedOutput.at(value%refocusedOutput.size()));
        [_image setImage:output];
    }
}

- (IBAction)angleStepper:(UIStepper *)sender {
    curMaxAngle = [sender value];
    [_maxAngle setText: [NSString stringWithFormat:@"max Angle: %d", curMaxAngle]];
}
@end
