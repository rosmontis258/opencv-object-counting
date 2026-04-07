#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "CodexProcess.h"

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

	const fs::path inputDir = "D:/Projects/opencv-object-counting/assets/input";
	const fs::path outputDir = "D:/Projects/opencv-object-counting/assets/output_codex";

	if (!fs::exists(inputDir))
	{
		cout << "[Codex] Input directory not found: " << inputDir.string() << endl;
		return -1;
	}

	fs::create_directories(outputDir);

	for (const auto& entry : fs::directory_iterator(inputDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const fs::path imagePath = entry.path();
		Mat image = imread(imagePath.string());
		if (image.empty())
		{
			cout << "[Codex] Fail to load image: " << imagePath.filename().string() << endl;
			continue;
		}

		const auto result = runCodexPipeline(image);
		saveCodexArtifacts(outputDir, imagePath.stem().string(), result);

		cout
			<< "[Codex] Processed "
			<< imagePath.filename().string()
			<< " | count = "
			<< result.detections.size()
			<< endl;
	}

	return 0;
}
