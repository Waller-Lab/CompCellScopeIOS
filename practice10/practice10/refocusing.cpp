
//
//  refocusing.cpp
//  practice10
//
//  Created by Joel Whang on 6/19/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#include "refocusing.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
#include <stdio.h>
#include <dirent.h>
#include "RefocusingCoordinates.h"
#include <CoreGraphics/CGImage.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGBitmapContext.h>
#include <dispatch/dispatch.h>
#include <math.h>


//#include "omp.h"
/* Tried to integrate OpenMP, doesn't really work since OpenCV has a lot
 of critical sections. */

#define FILENAME_LENGTH 32
#define FILE_HOLENUM_DIGITS 3
using namespace std;
using namespace cv;


/* Produces a set of images from zMin to zMax, incrementing by zStep.
 All measured in microns. */
int zMin, zMax, zStep;
string datasetRoot;//folder of input images

class R_image{
    
public:
    CGImageRef Image;
    int led_num;
    float tan_x;
    float tan_y;
};

cv::Mat CGImageRef2Mat(CGImageRef image) {
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(image);
    size_t cols = CGImageGetHeight(image);
    size_t rows = CGImageGetWidth(image);
    
    cv::Mat cvMat(rows, cols, CV_8UC4); // 8 bits per component, 4 channels (color channels + alpha
    CGContextRef contextRef = CGBitmapContextCreate(cvMat.data,                 // Pointer to  data
                                                    cols,                       // Width of bitmap
                                                    rows,                       // Height of bitmap
                                                    8,                          // Bits per component
                                                    cvMat.step[0],              // Bytes per row
                                                    colorSpace,                 // Colorspace
                                                    kCGImageAlphaNoneSkipLast |
                                                    kCGBitmapByteOrderDefault); // Bitmap info flags
    
    CGContextDrawImage(contextRef, CGRectMake(0, 0, cols, rows), image);
    CGContextRelease(contextRef);
    
    return cvMat;
}




void circularShift(Mat img, Mat result, int x, int y){
    int w = img.cols;
    int h  = img.rows;
    
    int shiftR = x % w;
    int shiftD = y % h;
    
    if (shiftR < 0)//if want to shift in -x direction
        shiftR += w;
    
    if (shiftD < 0)//if want to shift in -y direction
        shiftD += h;
    
    cv::Rect gate1(0, 0, w-shiftR, h-shiftD);//rect(x, y, width, height)
    cv::Rect out1(shiftR, shiftD, w-shiftR, h-shiftD);
    
    cv::Rect gate2(w-shiftR, 0, shiftR, h-shiftD);
    cv::Rect out2(0, shiftD, shiftR, h-shiftD);
    
    cv::Rect gate3(0, h-shiftD, w-shiftR, shiftD);
    cv::Rect out3(shiftR, 0, w-shiftR, shiftD);
    
    cv::Rect gate4(w-shiftR, h-shiftD, shiftR, shiftD);
    cv::Rect out4(0, 0, shiftR, shiftD);
    
    cv::Mat shift1 = img ( gate1 );
    cv::Mat shift2 = img ( gate2 );
    cv::Mat shift3 = img ( gate3 );
    cv::Mat shift4 = img ( gate4 );
    
    shift1.copyTo(cv::Mat(result, out1));//copyTo will fail if any rect dimension is 0
    if(shiftR != 0)
        shift2.copyTo(cv::Mat(result, out2));
    if(shiftD != 0)
        shift3.copyTo(cv::Mat(result, out3));
    if(shiftD != 0 && shiftR != 0)
        shift4.copyTo(cv::Mat(result, out4));
}


int loadImages(vector<string> *ref, vector<CGImageRef> *input, vector<R_image> *output){
    int vectorSize = (*input).size();
    for (int idx = 0; idx<vectorSize; idx++) {
        R_image currentImage;
        string fileName = (*ref).at(idx);
        string filePrefix = "_scanning_";
        string holeNum = fileName.substr(fileName.find(filePrefix)+filePrefix.length(),FILE_HOLENUM_DIGITS);
        currentImage.Image = (*input).at(idx);
        currentImage.led_num = atoi(holeNum.c_str());
        currentImage.tan_x = domeCoordinates[currentImage.led_num][0] / domeCoordinates[currentImage.led_num][2];
        currentImage.tan_y = domeCoordinates[currentImage.led_num][1] / domeCoordinates[currentImage.led_num][2];
        (*output).push_back(currentImage);
    }
    return vectorSize;
}


void computeFocusDPC(vector<R_image> *iStack, int fileCount, float z, int width, int height, int xcrop, int ycrop, Mat* results, double maxAngle) {
    int newWidth = width;// - 2*xcrop;
    int newHeight = height;// - 2*ycrop;
    
    cv::Mat bf_result = cv::Mat(newHeight, newWidth, CV_16UC3, double(0));
    cv::Mat dpc_result_tb = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    cv::Mat dpc_result_lr = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    
    cv::Mat bf_result_1 = cv::Mat(newHeight, newWidth, CV_16UC3, double(0));
    cv::Mat dpc_result_tb_1 = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    cv::Mat dpc_result_lr_1 = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    
    cv::Mat bf_result_2 = cv::Mat(newHeight, newWidth, CV_16UC3, double(0));
    cv::Mat dpc_result_tb_2 = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    cv::Mat dpc_result_lr_2 = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
    
    cv::Mat bf_result8 = cv::Mat(newHeight, newWidth, CV_8UC3);
    cv::Mat dpc_result_tb8 = cv::Mat(newHeight, newWidth, CV_8UC1);
    cv::Mat dpc_result_lr8 = cv::Mat(newHeight, newWidth, CV_8UC1);
    
    double tan_max = tan(maxAngle*M_PI/180); //tan of 10 degrees
    
    __block int leftCount1=0;
    __block int leftCount2=0;
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_apply(2, queue, ^(size_t loop){
        if (loop == 0){
            cv::Mat img;
            cv::Mat img16;
            cv::Mat shifted = cv::Mat(width, height, CV_16UC3,double(0));
            vector<Mat> channels(3);
            for (int idx = 0; idx < fileCount/2; idx++)
            {
                double tan_x = (*iStack)[idx].tan_x;
                double tan_y = (*iStack)[idx].tan_y;
                double tanCalc = sqrt(pow(tan_x,2)+pow(tan_y,2));
                if (tan_max > tanCalc){
                    // convert CGImageRef to cv::Mat, convert to 16 bit grayscale image
                    img = CGImageRef2Mat((*iStack)[idx].Image).t();
                    cv::cvtColor(img, img, COLOR_BGRA2BGR);

                    // Get home number
                    int holeNum = (*iStack)[idx].led_num;
                    
                    // Calculate shift based on array coordinates and desired z-distance
                    int xShift = (int) round(z*(*iStack)[idx].tan_x);
                    int yShift = (int) round(z*(*iStack)[idx].tan_y);
                    circularShift(img, shifted, yShift, xShift);
                    shifted = shifted.t();
                    
                    // Add Brightfield image
                    cv::add(bf_result_1, shifted, bf_result_1);


                    // Convert shifted to b/w for DPC
                    split(shifted, channels);
                    shifted = shifted.t();
                    
                    channels[1].convertTo(channels[1],dpc_result_lr_1.type());
                    
                    if (std::find(std::begin(leftList),std::end(leftList),holeNum) != std::end(leftList)){
                        cv::add(dpc_result_lr_1, channels[1], dpc_result_lr_1);
                        leftCount1++;
                    }
                    else{
                        cv::subtract(dpc_result_lr_1, channels[1], dpc_result_lr_1);
                    }

                    if (std::find(std::begin(topList),std::end(topList),holeNum) != std::end(topList)) {
                        cv::add(dpc_result_tb_1, channels[1], dpc_result_tb_1);
                        leftCount1++;
                    }
                    else{
                        cv::subtract(dpc_result_tb_1, channels[1], dpc_result_tb_1);
                    }
                }
            }
        }
        else if(loop == 1) {
            cv::Mat img;
            cv::Mat img16;
            cv::Mat shifted = cv::Mat(width, height, CV_16UC3,double(0));
            vector<Mat> channels(3);
            for (int idx = fileCount/2+1; idx < fileCount; idx++)
            {
                double tan_x = (*iStack)[idx].tan_x;
                double tan_y = (*iStack)[idx].tan_y;
                double tanCalc = sqrt(pow(tan_x,2)+pow(tan_y,2));

                if (tan_max > tanCalc){
                    // convert CGImageRef to cv::Mat, convert to 16 bit grayscale image
                    img = CGImageRef2Mat((*iStack)[idx].Image).t();
                    cv::cvtColor(img, img, COLOR_BGRA2BGR);
                    
                    // Get home number
                    int holeNum = (*iStack)[idx].led_num;
                    
                    // Calculate shift based on array coordinates and desired z-distance
                    int xShift = (int) round(z*(*iStack)[idx].tan_x);
                    int yShift = (int) round(z*(*iStack)[idx].tan_y);
                    circularShift(img, shifted, yShift, xShift);
                    shifted = shifted.t();
                    // Add Brightfield image
                    cv::add(bf_result_2, shifted, bf_result_2);
                    
                    
                    // Convert shifted to b/w for DPC
                    split(shifted, channels);
                    shifted = shifted.t();
                    
                    channels[1].convertTo(channels[1],dpc_result_lr_2.type());
                    
                    if (std::find(std::begin(leftList),std::end(leftList),holeNum) != std::end(leftList)){
                        cv::add(dpc_result_lr_2, channels[1], dpc_result_lr_2);
                        leftCount2++;
                    }
                    else{
                        cv::subtract(dpc_result_lr_2, channels[1], dpc_result_lr_2);}
                    
                    if (std::find(std::begin(topList),std::end(topList),holeNum) != std::end(topList)){
                        cv::add(dpc_result_tb_2, channels[1], dpc_result_tb_2);
                        leftCount2++;
                    }
                    else{
                        cv::subtract(dpc_result_tb_2, channels[1], dpc_result_tb_2);
                    }
                }
            }
        }
    });
    


    //combine results from both threads
    cv::add(bf_result_1, bf_result_2, bf_result);
    cv::add(dpc_result_lr_1, dpc_result_lr_2, dpc_result_lr);
    cv::add(dpc_result_tb_1, dpc_result_tb_2, dpc_result_tb);
        
    
    // Scale the values to 8-bit images
    double min_1, max_1, min_2, max_2, min_3, max_3;
    
    cv::minMaxLoc(bf_result, &min_1, &max_1);
    bf_result.convertTo(bf_result8, CV_8UC4, 255/(max_1 - min_1), - min_1 * 255.0/(max_1 - min_1));
    
    cv::minMaxLoc(dpc_result_lr.reshape(1), &min_2, &max_2);
    dpc_result_lr.convertTo(dpc_result_lr8, CV_8UC4, 255/(max_2 - min_2), -min_2 * 255.0/(max_2 - min_2));
    
    cv::minMaxLoc(dpc_result_tb.reshape(1), &min_3, &max_3);
    dpc_result_tb.convertTo(dpc_result_tb8, CV_8UC4, 255/(max_3 - min_3), -min_3 * 255.0/(max_3 - min_3));

    
    
    results[0] = bf_result8;
    results[1] = dpc_result_lr8;
    results[2] = dpc_result_tb8;
    
    for (int x = 0; x < 3; x+=1) {
        resize(results[x], results[x], cv::Size(results[x].cols/4, results[x].rows/4));
    }
}
/* Compile with ./compile.sh */
std::vector<cv::Mat> refocus(std::vector<std::string> *refVector, std::vector<CGImageRef> *imageVector, double maxAngle)
{
    int zMin = -100;
    int zStep = 25;
    int zMax = 100;
    
    vector<cv::Mat> resultsVector;
    vector<R_image> * imageStack;
    imageStack = new vector<R_image>;
    int16_t imgCount = loadImages(refVector, imageVector, imageStack);

    cv::Mat reference = CGImageRef2Mat((*imageVector).at(0));
    int cols = reference.cols;
    int rows = reference.rows;
    
    int vectorCounter = 0;
    
    for (int zDist = zMin; zDist <= zMax; zDist += zStep) {
        Mat results[3];
   	    computeFocusDPC(imageStack, imgCount, zDist, cols, rows, 0, 0, results, maxAngle);
        
        for (int i = 0; i < 3; i++) {
            resultsVector.insert(resultsVector.begin() + vectorCounter, results[i]);
            vectorCounter++;
        }

    }

    
    (*imageStack).clear();
    
    return resultsVector;
}