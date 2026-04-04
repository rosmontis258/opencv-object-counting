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

	// 修改：这里直接走你已经封装好的整体分割流程。
	Mat mask = craveProcess(img);

	// 修改：在分割结果的基础上继续做轮廓过滤和画框。
	Mat result = contoursProcess(mask, img);

	Mat maskResize;
	resize(mask, maskResize, Size(), 0.1, 0.1);
	imshow("mask", maskResize);

	Mat resultResize;
	resize(result, resultResize, Size(), 0.1, 0.1);
	imshow("result", resultResize);
	waitKey(0);
	return 0;
}
