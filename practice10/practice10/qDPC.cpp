//
//  qDPC.cpp
//  practice10
//
//  Created by Joel Whang on 6/12/15.
//  Copyright (c) 2015 Joel Whang. All rights reserved.
//

#include "qDPC.h"

/* C++ program to calculate the quantative DPC
 Updated 5/5/2015
 */

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <iostream>
#include <string>
#include <stdio.h>
#include <dirent.h>
#include <complex.h>
#include <ctime>
#include <unordered_map>
#include <sys/stat.h>
#include <dispatch/dispatch.h>


using namespace std;
using namespace cv;

#include "domeCoordinates.h"

//Check if file exists
inline bool exists_test (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

/*Calculate the DPC of two images */
cv::Mat calcDPC( cv::Mat image1, cv::Mat image2)
{
    
    cv::cvtColor(image1,image1,COLOR_BGRA2GRAY);//convert to grayscale
    cv::cvtColor(image2,image2,COLOR_BGRA2GRAY);
    
    
    image1.convertTo(image1,CV_32FC1);
    image2.convertTo(image2,CV_32FC1);
    
    
    cv::Mat tmp1, tmp2;
    cv::subtract(image1, image2, tmp1);
    cv::add(image1, image2, tmp2);
    
    cv::Mat output;
    cv::divide(tmp1,tmp2,output);
    
    // Crop the ROI to a circle to get rid of noise
    int16_t circPad = 100;
    cv::Mat circMask(output.size(),CV_32FC1,cv::Scalar(0));
    cv::Point center(cvRound(output.rows/2),cvRound(output.cols/2));
    cv::circle(circMask, center, cvRound(output.rows/2)-circPad ,cv::Scalar(1.0), -1,8,0);
    cv::multiply(output,circMask,output);
    
    
    //normalize
    double min_1, max_1;
    cv::minMaxLoc(output, &min_1, &max_1);
    output.convertTo(output, CV_8UC1, 255/(max_1 - min_1), - min_1 * 255.0/(max_1 - min_1));
    
    return output;
}

void assignAtIndex(int i, int j,complex<double> result, cv::Mat real, cv::Mat imag) {
    real.at<double>(i, j) = result.real();
    imag.at<double>(i, j) = result.imag();
}

cv::Mat qDPC_loop(vector<cv::Mat>dpcList, vector<cv::Mat>transferFunctionList, double reg)
{
    
    vector<Mat> complexPlanes_idx1;
    vector<Mat> complexPlanes_idx2;
    Mat planes[2];
    
    // Initialize Results
    Mat ph_dpc_ft_real = Mat::zeros(dpcList.at(0).size(), CV_64F);
    Mat ph_dpc_ft_imag = Mat::zeros(dpcList.at(0).size(), CV_64F);
    Mat dpcComplex, tmp;
    
    vector<cv::Mat> dpcFtRealList(dpcList.size());
    vector<cv::Mat> dpcFtImagList(dpcList.size());
    
    
    Mat padded;
    //expand input image to optimal size
    int m = getOptimalDFTSize( dpcList.at(0).rows );
    int n = getOptimalDFTSize( dpcList.at(0).cols ); // on the border add zero values
    
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    for (int idx = 0; idx<transferFunctionList.size(); idx++)
        {
        
        //void copyMakeBorder(InputArray src, OutputArray dst, int top, int bottom, int left, int right, int borderType, const Scalar& value=Scalar() )
        copyMakeBorder(transferFunctionList.at(idx), padded, 0, m - transferFunctionList.at(idx).rows, 0, n - transferFunctionList.at(idx).cols, BORDER_CONSTANT, Scalar::all(0));
        copyMakeBorder(dpcList.at(idx), padded, 0, m - dpcList.at(idx).rows, 0, n - dpcList.at(idx).cols, BORDER_CONSTANT, Scalar::all(0));
        
        // Take Fourier Transforms of DPC Images
        planes[0] = Mat_<double>(dpcList.at(idx));
        planes[1] = Mat::zeros(dpcList.at(idx).size(), CV_64F);
        
        //merge array of Mats into one Mat. 2 refers to size of array.
        merge(planes, 2, dpcComplex);
        dft(dpcComplex, dpcComplex);
        
        if (idx==0){
            split(dpcComplex, complexPlanes_idx1);
            dpcFtRealList.at(idx) = complexPlanes_idx1[0];
            dpcFtImagList.at(idx) = complexPlanes_idx1[1];
        } else if (idx == 1) {
            split(dpcComplex, complexPlanes_idx2);
            dpcFtRealList.at(idx) = complexPlanes_idx2[0];
            dpcFtImagList.at(idx) = complexPlanes_idx2[1];
        }
    }


    
    // Try all of this inside of a loop to properly deal with complex values
    //loop through each pixel (i,j)
    dispatch_apply(dpcComplex.cols, queue, ^(size_t l){
        for (int j=0; j < dpcComplex.rows; j++){
            int i = (int) l;

            complex<double> H_sum = std::complex<double>(0,0);
            complex<double> Hi_sum = std::complex<double>(0,0);
            complex<double> I_sum = std::complex<double>(0,0);
            complex<double> result;
            
            //get value of each transfer function at (i,j)
            H_sum += std::complex<double>(0,transferFunctionList.at(0).at<double>(i,j));
            Hi_sum += std::complex<double>(0,-1*transferFunctionList.at(0).at<double>(i,j));
            I_sum += std::complex<double>(dpcFtRealList.at(0).at<double>(i,j),dpcFtImagList.at(0).at<double>(i,j));
            
            H_sum += std::complex<double>(0,transferFunctionList.at(1).at<double>(i,j));
            Hi_sum += std::complex<double>(0,-1*transferFunctionList.at(1).at<double>(i,j));
            I_sum += std::complex<double>(dpcFtRealList.at(1).at<double>(i,j),dpcFtImagList.at(1).at<double>(i,j));
            
            result = ((I_sum*Hi_sum)/(abs(H_sum)*abs(H_sum) + reg));
            
            assignAtIndex(i, j, result, ph_dpc_ft_real, ph_dpc_ft_imag);
        }
    });

    
    
    cv::Mat diff = complexPlanes_idx1[0] != complexPlanes_idx2[0];
    Mat ph_complex_ft, ph_complex;
    planes[0] = ph_dpc_ft_real;
    planes[1] = ph_dpc_ft_imag;
    
    
    merge(planes, 2, ph_complex_ft);
    dft(ph_complex_ft, ph_complex, DFT_INVERSE);
    split(ph_complex, complexPlanes_idx2);

    return complexPlanes_idx2[0]; // Real Part
}



Mat dpc(Mat dpc11, Mat dpc12, Mat dpc21, Mat dpc22, Mat trans1, Mat trans2, double reg)
{
    trans1.convertTo(trans1,CV_64FC1);//CV_64FC1: 64-bit, float, 1 channel
    trans2.convertTo(trans2,CV_64FC1);
    
    normalize(trans1, trans1, -1, 1, CV_MINMAX);
    normalize(trans2, trans2, -1, 1, CV_MINMAX);
    
    vector<cv::Mat> transferFunctionList;
    transferFunctionList.push_back(trans1);
    transferFunctionList.push_back(trans2);
    
    // Crop to square
    dpc11 = dpc11 (CellScopeCropHorz);
    dpc12 = dpc12 (CellScopeCropHorz);
    dpc21 = dpc21 (CellScopeCropHorz);
    dpc22 = dpc22 (CellScopeCropHorz);
    
    //Compute DPC Images
    cv::Mat dpc1 = calcDPC(dpc11,dpc12);
    cv::Mat dpc2 = calcDPC(dpc21,dpc22);
    
    
    vector<cv::Mat> dpcList;
    dpcList.push_back(dpc1);
    dpcList.push_back(dpc2);
    
    Mat ph_dpc = qDPC_loop(dpcList,transferFunctionList,reg);
    normalize(ph_dpc, ph_dpc, -0.3, 1.4, CV_MINMAX);
    
    Mat cm_ph_dpc;
    cv::applyColorMap(ph_dpc, cm_ph_dpc, COLORMAP_COOL);
    
    ph_dpc.convertTo(ph_dpc, CV_32F);
    cv::cvtColor(ph_dpc, ph_dpc, COLOR_GRAY2BGRA);

    //must convert Mat type to CV_8UC1 for UIIMageView to take in
    double minVal, maxVal;
    minMaxLoc(ph_dpc, &minVal, &maxVal);
    Mat draw;
    ph_dpc.convertTo(draw, CV_8U, 255.0/(maxVal - minVal), -minVal*255.0/(maxVal - minVal));
//    ph_dpc.convertTo(ph_dpc, CV_8UC4);

    
    return draw;
}

