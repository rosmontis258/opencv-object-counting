#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Mat colorProcess(const Mat& img)
{
	//原图转HSV
	Mat imgHsv;
	cvtColor(img, imgHsv, COLOR_BGR2HSV);
	Mat result = img.clone();

	//HSV转二值化并反色
	Mat mask;
	inRange(imgHsv, Scalar(162, 20, 131), Scalar(179, 255, 255), mask);
	bitwise_not(mask, mask);
	
	//分水岭，找中心点
	Mat dist, distNorm;
	distanceTransform(mask, dist, DIST_L2, 5);
	normalize(dist, distNorm, 0, 1.0, NORM_MINMAX);

	// 提取确定前景
	Mat sureFg;
	threshold(distNorm, sureFg, 0.5, 1.0, THRESH_BINARY);
	sureFg.convertTo(sureFg, CV_8U, 255);

	// 提取确定背景
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	Mat sureBg;
	dilate(mask, sureBg, kernel, Point(-1, -1), 3);

	// unknown = 背景 - 前景
	Mat unknown;
	subtract(sureBg, sureFg, unknown);

	// 连通域标记
	Mat markers;
	connectedComponents(sureFg, markers);

	// 分水岭要求背景不是0，所以整体+1
	markers = markers + 1;

	// unknown 区域标成0
	for (int i = 0; i < unknown.rows; i++) {
		for (int j = 0; j < unknown.cols; j++) {
			if (unknown.at<uchar>(i, j) == 255) {
				markers.at<int>(i, j) = 0;
			}
		}
	}

	// 原图必须是三通道
	Mat colorImg = img.clone();
	watershed(colorImg, markers);

	double minVal, maxVal;
	minMaxLoc(markers, &minVal, &maxVal);

	int count = 0;

	for (int label = 2; label <= static_cast<int>(maxVal); ++label)
	{
		Mat objMask = (markers == label);   // 取出某一个物体区域
		objMask.convertTo(objMask, CV_8U, 255);

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(objMask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		if (contours.empty()) continue;

		double area = contourArea(contours[0]);
		if (area < 100) continue;   // 过滤小噪声

		Rect box = boundingRect(contours[0]);
		rectangle(result, box, Scalar(0, 255, 0), 10);
		count++;
	}

	cout << count << endl;
	return result;
}