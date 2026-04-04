#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;


int main(int argc, char* argv[])
{
	string imagePath = "D:/Projects/opencv-object-counting/assets/input/pill2.jpg";
	if (argc >= 2)imagePath = argv[1];
	Mat img = imread(imagePath);
	if (img.empty())
	{
		cout << "Fail to load image" << endl;
		return -1;
	}
	Mat mask = craveProcess(img);
	Mat imgContours = contoursProcess(mask, img);
	Mat imgSize;
	resize(imgContours, imgSize, Size(), 0.1, 0.1);
	imshow("test01", imgSize);
	waitKey(0);
	return 0;
}


/*
Mat img, imgHsv, mask, imgSize;
int h_min = 0, s_min = 0, v_min = 0;
int h_max = 179, s_max = 255, v_max = 255;

void update(int, void*)
{
	inRange(imgHsv,
		Scalar(h_min, s_min, v_min),
		Scalar(h_max, s_max, v_max),
		mask);
	Mat imgSize;
	bitwise_not(mask, mask);
	imshow("mask", mask);
}
int main()
{
	img = imread("D:/Projects/opencv-object-counting/assets/input/pill2.jpg");
	resize(img, imgSize, Size(), 0.1, 0.1);
	cvtColor(imgSize, imgHsv, COLOR_BGR2HSV);

	namedWindow("mask");
	createTrackbar("h_min", "mask", &h_min, 179, update);
	createTrackbar("h_max", "mask", &h_max, 179, update);
	createTrackbar("s_min", "mask", &s_min, 255, update);
	createTrackbar("s_max", "mask", &s_max, 255, update);
	createTrackbar("v_min", "mask", &v_min, 255, update);
	createTrackbar("v_max", "mask", &v_max, 255, update);

	update(0, 0);
	waitKey(0);
	return 0;
}
*/