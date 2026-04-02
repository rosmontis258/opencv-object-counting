#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
	string path = "D:/Projects/opencv-object-counting/assets/input/coin2.jpg";
	Mat img = imread(path);
	Mat imgS = judgeBasicType(img);
	Mat imgResize;
	resize(imgS, imgResize, Size(), 0.1, 0.1);
	imshow("testH", imgResize);
	waitKey(0);
	return 0;
}