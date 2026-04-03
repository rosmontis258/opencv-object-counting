#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

//选定基底方法
Mat judgeBasicType(const Mat& img);

//基底转二值化
Mat basic2Binary(const Mat& img);

//选定是否分割
bool judgeCrave(const Mat& img);

//分水岭法分割
Mat craveType(const Mat& img);