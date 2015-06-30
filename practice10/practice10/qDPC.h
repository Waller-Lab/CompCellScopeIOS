//
//  qDPC.h
//  practice10
//
//  Created by Joel Whang on 6/12/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#include <stdio.h>
#include "opencv2/core/core.hpp"
#include <CoreGraphics/CGImage.h>
#ifdef __cplusplus
extern "C" {
#endif
    
    cv::Mat dpc(cv::Mat left, cv::Mat right, cv::Mat top,\
                cv::Mat bottom, cv::Mat transfer1, cv::Mat transfer2, double reg);
    
#ifdef __cplusplus
}
#endif /* defined(__practice10__qDPC__) */


