#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Mat judgeBasicType(const Mat& img)
{
	Mat imgHSV;
	cvtColor(img, imgHSV, COLOR_BGR2HSV);
	
	vector<Mat> channels;
	split(imgHSV, channels);
	Mat imgS = channels[1];
	Scalar meanS = mean(imgS);
	double sAvg = meanS[0];
	
	if (sAvg > 53.2)
	{
		return imgS;
	}
	else {
		Mat imgGray = cvtColor(img, imgGray, COLOR_BGR2GRAY);
		return imgGray;
	}
}

Mat judgeCrave(const Mat& img)
{
	Mat imgBinary;
	threshold(img, imgBinary, 0, 255, THRESH_BINARY | THRESH_OTSU);

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(imgBinary, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	double area = 0, avgArea = 0;
	for (int i = 0; i < contours.size(); ++i)
		area += contourArea(contours[i]);
	avgArea = area / contours.size();
}