#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Mat colorProcess(const Mat& img)
{
	Mat imgHsv;
	cvtColor(img, imgHsv, COLOR_BGR2HSV);
	Mat result = img.clone();

	Mat mask;
	inRange(imgHsv, Scalar(162, 20, 131), Scalar(179, 255, 255), mask);
	bitwise_not(mask, mask);
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	erode(mask, mask, kernel, Point(-1, -1), 1);

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < contours.size(); ++i)
	{
		Rect box = boundingRect(contours[i]);
		rectangle(result, box, Scalar(0, 255, 0), 10);
	}
	cout << contours.size() << endl;
	return result;
}