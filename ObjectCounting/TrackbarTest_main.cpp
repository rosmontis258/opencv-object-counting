#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;

namespace
{
Mat g_originalImg;
Mat g_basicImg;
Mat g_binaryImg;

// 手动调 basicImg2Binary 阈值时使用的参数。
int g_thresholdValue = 127;
int g_invertBinary = 0;

void updateBinaryPreview(int, void*)
{
	if (g_basicImg.empty())
	{
		return;
	}

	int thresholdType = THRESH_BINARY;
	if (g_invertBinary == 1)
	{
		thresholdType = THRESH_BINARY_INV;
	}

	// 这里直接模拟 basicImg2Binary 的核心步骤，但把阈值改成手动调节。
	threshold(g_basicImg, g_binaryImg, g_thresholdValue, 255, thresholdType);

	Mat originalPreview;
	Mat basicPreview;
	Mat binaryPreview;

	resize(g_originalImg, originalPreview, Size(), 0.2, 0.2);
	resize(g_basicImg, basicPreview, Size(), 0.2, 0.2);
	resize(g_binaryImg, binaryPreview, Size(), 0.2, 0.2);

	imshow("original", originalPreview);
	imshow("basic", basicPreview);
	imshow("binary", binaryPreview);
}
}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

	// 这里固定成一张测试图，你后面手动改这个路径就行。
	string imagePath = "D:/Projects/opencv-object-counting/assets/input/pill2.jpg";

	g_originalImg = imread(imagePath);
	if (g_originalImg.empty())
	{
		cout << "Fail to load image: " << imagePath << endl;
		return -1;
	}

	// 先走基础分支判断，得到 basicImg2Binary 真正要处理的图。
	g_basicImg = judgeBasicType(g_originalImg);

	namedWindow("binary");
	createTrackbar("threshold", "binary", &g_thresholdValue, 255, updateBinaryPreview);
	createTrackbar("invert", "binary", &g_invertBinary, 1, updateBinaryPreview);

	// 初始化一次显示。
	updateBinaryPreview(0, nullptr);

	waitKey(0);
	return 0;
}
