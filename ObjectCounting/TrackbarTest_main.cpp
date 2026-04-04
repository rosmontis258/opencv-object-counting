#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
	string path = "D:/Projects/opencv-object-counting/assets/input/coin1.jpg";
	Mat img = imread(path);
	if (img.empty())
	{
		cout << "Fail to load image" << endl;
		return -1;
	}

	// 第一步：先选出更适合做基础分割的图像形式。
	Mat baseImg = judgeBasicType(img);

	// 第二步：得到统一极性的初始二值图。
	Mat imgBinary = basicImg2Binary(baseImg);

	// 第三步：只有疑似粘连时才进入分水岭。
	bool needCrave = judgeCrave(imgBinary);
	Mat result = imgBinary;
	if (needCrave)
	{
		result = craveType(img, imgBinary);
	}

	Mat imgResize;
	resize(result, imgResize, Size(), 0.1, 0.1);
	imshow("testH", imgResize);
	waitKey(0);
	return 0;
}
