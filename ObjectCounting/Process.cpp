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
		Mat imgGray;
		cvtColor(img, imgGray, COLOR_BGR2GRAY);
		return imgGray;
	}
}

Mat basicImg2Binary(const Mat& img)
{
	Mat imgBinary;
	threshold(img, imgBinary, 0, 255, THRESH_BINARY | THRESH_OTSU);
	return imgBinary;
}

bool judgeCrave(const Mat& imgBinary)
{
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(img, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	double totalArea = 0, avgArea = 0, maxArea = 0;
	for (int i = 0; i < contours.size(); ++i)
	{
		double area = contourArea(contours[i]);
		totalArea += area;
		maxArea = max(maxArea, area);
	}	
	avgArea = totalArea / contours.size();
	if (maxArea > 2 * avgArea)
		return true;
	else
		return false;
}

Mat craveType(const Mat& img)
{
	Mat imgBlur, imgBinary;
	Scalar kernel_size(3, 3);
	GaussianBlur(img, imgBlur, kernel_size, sigmaX = 0.5, sigmaY = 0.5);
	threshold(imgBlur, imgBinary, 0, 255, THRESH_BINARY | THRESH_OTSU);
}