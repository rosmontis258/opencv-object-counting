#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

//颜色对比识别
Mat colorProcess(const Mat& img);