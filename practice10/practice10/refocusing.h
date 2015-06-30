//
//  refocusing.h
//  practice10
//
//  Created by Joel Whang on 6/19/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#include <stdio.h>
#include "opencv2/core/core.hpp"
#include <CoreGraphics/CGImage.h>

#ifdef __cplusplus
extern "C" {
#endif
    std::vector<cv::Mat> refocus(std::vector<std::string> *refVector, std::vector<CGImageRef> *imageVector, double maxAngle);
        
    cv::Mat CGImageRef2Mat(CGImageRef image);

#ifdef __cplusplus
}
#endif /* defined(__practice10__qDPC__) */

