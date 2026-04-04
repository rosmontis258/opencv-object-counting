#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;

namespace
{
const double S_THRESHOLD = 53.2;
const double CRAVE_RATIO_THRESHOLD = 2.0;

// 统一二值图的极性：尽量保证白色是目标，黑色是背景。
Mat normalizeBinaryMask(const Mat& imgBinary)
{
	const double whiteRatio = static_cast<double>(countNonZero(imgBinary)) /
		static_cast<double>(imgBinary.rows * imgBinary.cols);

	if (whiteRatio > 0.5)
	{
		Mat inverted;
		bitwise_not(imgBinary, inverted);
		return inverted;
	}

	return imgBinary.clone();
}
}

Mat judgeBasicType(const Mat& img)
{
	Mat imgHSV;
	cvtColor(img, imgHSV, COLOR_BGR2HSV);
	
	vector<Mat> channels;
	split(imgHSV, channels);
	Mat imgS = channels[1];
	Scalar meanS = mean(imgS);
	double sAvg = meanS[0];
	cout << "mean S = " << sAvg << endl;
	
	if (sAvg > S_THRESHOLD)
	{
		// 颜色信息足够明显时，后续优先走 S 通道分支。
		return imgS;
	}
	else
	{
		// 颜色信息不明显时，退回灰度分支。
		Mat imgGray;
		cvtColor(img, imgGray, COLOR_BGR2GRAY);
		return imgGray;
	}
}

Mat basicImg2Binary(const Mat& img)
{
	Mat imgBinary;
	threshold(img, imgBinary, 0, 255, THRESH_BINARY | THRESH_OTSU);

	// 二值化后先统一黑白方向，后面的面积判断和分水岭都依赖这一点。
	return normalizeBinaryMask(imgBinary);
}

bool judgeCrave(const Mat& imgBinary)
{
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(imgBinary, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	if (contours.empty())
	{
		return false;
	}

	double totalArea = 0.0;
	double maxArea = 0.0;
	for (int i = 0; i < static_cast<int>(contours.size()); ++i)
	{
		double area = contourArea(contours[i]);
		totalArea += area;
		maxArea = max(maxArea, area);
	}

	const double avgArea = totalArea / static_cast<double>(contours.size());
	cout << "avgArea = " << avgArea << ", maxArea = " << maxArea << endl;

	return maxArea > CRAVE_RATIO_THRESHOLD * avgArea;
}

Mat craveType(const Mat& originalImg, const Mat& imgBinary)
{
	// 进入分水岭前，先复制一份已经统一极性的二值图。
	Mat workBinary = normalizeBinaryMask(imgBinary);

	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	morphologyEx(workBinary, workBinary, MORPH_OPEN, kernel);

	// sureBg: 通过膨胀得到更保守的背景区域。
	Mat sureBg;
	dilate(workBinary, sureBg, kernel, Point(-1, -1), 2);

	// 距离变换用于找每个目标的中心区域。
	Mat dist;
	distanceTransform(workBinary, dist, DIST_L2, 5);
	normalize(dist, dist, 0, 1.0, NORM_MINMAX);

	// sureFg: 通过阈值得到更确定的前景区域。
	Mat sureFg;
	threshold(dist, sureFg, 0.4, 1.0, THRESH_BINARY);
	sureFg.convertTo(sureFg, CV_8U, 255);

	// unknown 区域交给分水岭进一步判断归属。
	Mat unknown;
	subtract(sureBg, sureFg, unknown);

	Mat markers;
	connectedComponents(sureFg, markers);
	markers = markers + 1;
	markers.setTo(0, unknown == 255);

	// watershed 必须使用三通道原图，不能直接喂单通道 S 图或灰度图。
	Mat watershedInput = originalImg.clone();
	watershed(watershedInput, markers);

	// 把分水岭结果重新整理成二值 mask，方便后续继续提轮廓。
	Mat result = Mat::zeros(workBinary.size(), CV_8U);
	result.setTo(255, markers > 1);
	return result;
}
