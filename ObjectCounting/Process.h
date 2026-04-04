#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// 选择基础处理图。
// 如果图片颜色信息明显，就返回 S 通道图；
// 否则返回灰度图，方便后续统一做二值化。
Mat judgeBasicType(const Mat& img);

// 对基础处理图做第一次二值化，得到初始 mask。
Mat basicImg2Binary(const Mat& img);

// 根据轮廓面积分布判断是否需要分水岭拆分。
bool judgeCrave(const Mat& imgBinary);

// 对需要拆分的目标执行分水岭。
// originalImg 必须是三通道原图，imgBinary 是已经二值化后的 mask。
Mat craveType(const Mat& originalImg, const Mat& imgBinary);
