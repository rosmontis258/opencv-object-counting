#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
	string imagePath = "D:/Projects/opencv-object-counting/assets/input/pill1.jpg";
	if (argc >= 2)imagePath = argv[1];
	Mat img = imread(imagePath);
	if (img.empty())
	{
		cout << "Fail to load image" << endl;
		return -1;
	}
	imshow("test01", img);
	waitKey(0);
	return 0;
}