#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "Process.h"

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

	const fs::path inputDir = "D:/Projects/opencv-object-counting/assets/input";
	const fs::path outputDir = "D:/Projects/opencv-object-counting/assets/output";
	fs::create_directories(outputDir);

	for (const auto& entry : fs::directory_iterator(inputDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const fs::path imagePath = entry.path();
		Mat img = imread(imagePath.string());
		if (img.empty())
		{
			cout << "Fail to load image: " << imagePath.filename().string() << endl;
			continue;
		}

		Mat mask = craveProcess(img);
		Mat result = contoursProcess(mask, img);

		const string stem = imagePath.stem().string();
		imwrite((outputDir / (stem + "_mask.png")).string(), mask);
		imwrite((outputDir / (stem + "_result.png")).string(), result);
		cout << "Processed: " << imagePath.filename().string() << endl;
	}

	return 0;
}
