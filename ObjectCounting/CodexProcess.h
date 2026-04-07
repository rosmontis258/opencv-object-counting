#pragma once

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct CodexDetection
{
	int id = 0;
	double area = 0.0;
	double perimeter = 0.0;
	double circularity = 0.0;
	cv::Rect box;
	std::vector<cv::Point> contour;
};

struct CodexPipelineResult
{
	cv::Mat scoreMap;
	cv::Mat rawMask;
	cv::Mat cleanedMask;
	cv::Mat splitMask;
	cv::Mat markerPreview;
	cv::Mat annotated;
	std::vector<CodexDetection> detections;
	std::string strategyName;
};

CodexPipelineResult runCodexPipeline(const cv::Mat& image);

void saveCodexArtifacts(
	const std::filesystem::path& outputDir,
	const std::string& stem,
	const CodexPipelineResult& result);
