#include "CodexProcess.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>

using namespace cv;
using namespace std;

namespace
{
int clampOdd(int value, int minValue, int maxValue)
{
	value = max(minValue, min(value, maxValue));
	if (value % 2 == 0)
	{
		++value;
	}
	return value;
}

uchar medianOf(vector<uchar>& values)
{
	if (values.empty())
	{
		return 128;
	}

	const size_t mid = values.size() / 2;
	nth_element(values.begin(), values.begin() + mid, values.end());
	return values[mid];
}

Mat buildBorderMask(Size size)
{
	const int minDim = min(size.width, size.height);
	const int border = max(24, min(minDim / 18, 180));

	Mat mask(size, CV_8U, Scalar(255));
	if (size.width > border * 2 && size.height > border * 2)
	{
		rectangle(
			mask,
			Rect(border, border, size.width - border * 2, size.height - border * 2),
			Scalar(0),
			FILLED);
	}

	return mask;
}

Scalar estimateBackgroundLab(const Mat& bgr)
{
	Mat lab;
	cvtColor(bgr, lab, COLOR_BGR2Lab);

	const Mat borderMask = buildBorderMask(bgr.size());
	vector<uchar> lValues;
	vector<uchar> aValues;
	vector<uchar> bValues;
	lValues.reserve(countNonZero(borderMask));
	aValues.reserve(lValues.capacity());
	bValues.reserve(lValues.capacity());

	for (int y = 0; y < lab.rows; ++y)
	{
		const Vec3b* labRow = lab.ptr<Vec3b>(y);
		const uchar* maskRow = borderMask.ptr<uchar>(y);
		for (int x = 0; x < lab.cols; ++x)
		{
			if (maskRow[x] == 0)
			{
				continue;
			}

			lValues.push_back(labRow[x][0]);
			aValues.push_back(labRow[x][1]);
			bValues.push_back(labRow[x][2]);
		}
	}

	return Scalar(
		medianOf(lValues),
		medianOf(aValues),
		medianOf(bValues));
}

Mat normalizeFloatToU8(const Mat& src)
{
	Mat dst;
	normalize(src, dst, 0, 255, NORM_MINMAX, CV_8U);
	return dst;
}

Mat buildScoreMap(const Mat& bgr)
{
	Mat lab;
	cvtColor(bgr, lab, COLOR_BGR2Lab);

	Mat hsv;
	cvtColor(bgr, hsv, COLOR_BGR2HSV);

	vector<Mat> hsvChannels;
	split(hsv, hsvChannels);

	const Scalar bgLab = estimateBackgroundLab(bgr);

	Mat lab32;
	lab.convertTo(lab32, CV_32F);
	vector<Mat> lab32Channels;
	split(lab32, lab32Channels);

	Mat da = lab32Channels[1] - static_cast<float>(bgLab[1]);
	Mat db = lab32Channels[2] - static_cast<float>(bgLab[2]);
	Mat abDistance;
	magnitude(da, db, abDistance);

	Mat gray;
	cvtColor(bgr, gray, COLOR_BGR2GRAY);
	const int minDim = min(bgr.cols, bgr.rows);
	const int blurSize = clampOdd(minDim / 12, 31, 151);
	Mat backgroundGray;
	GaussianBlur(gray, backgroundGray, Size(blurSize, blurSize), 0.0);

	Mat localContrast;
	absdiff(gray, backgroundGray, localContrast);

	Mat abNorm = normalizeFloatToU8(abDistance);
	Mat contrastNorm = normalizeFloatToU8(localContrast);
	Mat satNorm = hsvChannels[1].clone();

	double abWeight = 0.62;
	double contrastWeight = 0.23;
	double satWeight = 0.15;

	if (mean(abNorm)[0] < 22.0)
	{
		abWeight = 0.48;
		contrastWeight = 0.34;
		satWeight = 0.18;
	}

	Mat score32 =
		abNorm * abWeight +
		contrastNorm * contrastWeight +
		satNorm * satWeight;

	return normalizeFloatToU8(score32);
}

Mat fillMaskHoles(const Mat& mask)
{
	Mat padded;
	copyMakeBorder(mask, padded, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0));

	Mat flood = padded.clone();
	floodFill(flood, Point(0, 0), Scalar(255));

	Mat floodNoBorder = flood(Rect(1, 1, mask.cols, mask.rows));
	Mat floodInv;
	bitwise_not(floodNoBorder, floodInv);

	Mat filled;
	bitwise_or(mask, floodInv, filled);
	return filled;
}

Mat keepLargeComponents(const Mat& mask, double minArea)
{
	Mat labels;
	Mat stats;
	Mat centroids;
	const int componentCount = connectedComponentsWithStats(
		mask,
		labels,
		stats,
		centroids,
		8,
		CV_32S);

	Mat kept(mask.size(), CV_8U, Scalar(0));
	for (int label = 1; label < componentCount; ++label)
	{
		const int area = stats.at<int>(label, CC_STAT_AREA);
		if (area < minArea)
		{
			continue;
		}

		kept.setTo(255, labels == label);
	}

	return kept;
}

double computeMedianComponentArea(const Mat& mask, double minArea)
{
	Mat labels;
	Mat stats;
	Mat centroids;
	const int componentCount = connectedComponentsWithStats(
		mask,
		labels,
		stats,
		centroids,
		8,
		CV_32S);

	vector<double> areas;
	for (int label = 1; label < componentCount; ++label)
	{
		const double area = static_cast<double>(stats.at<int>(label, CC_STAT_AREA));
		if (area >= minArea)
		{
			areas.push_back(area);
		}
	}

	if (areas.empty())
	{
		return max(minArea * 4.0, 1.0);
	}

	const size_t mid = areas.size() / 2;
	nth_element(areas.begin(), areas.begin() + mid, areas.end());
	return areas[mid];
}

Mat buildInitialMask(const Mat& scoreMap)
{
	Mat smoothed;
	GaussianBlur(scoreMap, smoothed, Size(5, 5), 0.0);

	Mat rawMask;
	threshold(smoothed, rawMask, 0, 255, THRESH_BINARY | THRESH_OTSU);
	return rawMask;
}

Mat cleanMask(const Mat& rawMask, double minArea)
{
	const int minDim = min(rawMask.cols, rawMask.rows);
	const int closeSize = clampOdd(minDim / 260, 3, 9);
	const int openSize = clampOdd(minDim / 340, 3, 7);
	const Mat closeKernel = getStructuringElement(MORPH_ELLIPSE, Size(closeSize, closeSize));
	const Mat openKernel = getStructuringElement(MORPH_ELLIPSE, Size(openSize, openSize));

	Mat cleaned = rawMask.clone();
	morphologyEx(cleaned, cleaned, MORPH_CLOSE, closeKernel);
	morphologyEx(cleaned, cleaned, MORPH_OPEN, openKernel);
	cleaned = fillMaskHoles(cleaned);
	cleaned = keepLargeComponents(cleaned, minArea);
	cleaned = fillMaskHoles(cleaned);
	return cleaned;
}

int estimateSeedCount(const Mat& localMask, int area, double medianArea, int width, int height)
{
	vector<vector<Point>> contours;
	findContours(localMask.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	double solidity = 1.0;
	double circularity = 1.0;
	if (!contours.empty())
	{
		const auto contour = *max_element(
			contours.begin(),
			contours.end(),
			[](const vector<Point>& lhs, const vector<Point>& rhs)
			{
				return contourArea(lhs) < contourArea(rhs);
			});

		vector<Point> hull;
		convexHull(contour, hull);
		const double contourAreaValue = contourArea(contour);
		const double hullArea = max(1.0, contourArea(hull));
		solidity = contourAreaValue / hullArea;

		const double perimeter = arcLength(contour, true);
		circularity = perimeter > 0.0
			? (4.0 * CV_PI * contourAreaValue) / (perimeter * perimeter)
			: 1.0;
	}

	int expected = max(
		1,
		min(10, static_cast<int>(round(area / max(medianArea, 1.0)))));

	const double aspect = static_cast<double>(max(width, height)) /
		static_cast<double>(max(1, min(width, height)));
	if (solidity > 0.94 && circularity > 0.70)
	{
		return 1;
	}

	if (aspect > 2.2 && solidity > 0.90)
	{
		return 1;
	}

	return expected;
}

Mat buildSeedsByErosion(const Mat& localMask, int expectedCount)
{
	if (expectedCount <= 1)
	{
		return Mat();
	}

	const Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	Mat eroded = localMask.clone();
	Mat bestSeedMask;
	int bestCount = 1;

	for (int iter = 0; iter < 12; ++iter)
	{
		erode(eroded, eroded, kernel);
		if (countNonZero(eroded) == 0)
		{
			break;
		}

		Mat labels;
		Mat stats;
		Mat centroids;
		const int componentCount = connectedComponentsWithStats(
			eroded,
			labels,
			stats,
			centroids,
			8,
			CV_32S) - 1;

		if (componentCount <= 1)
		{
			continue;
		}

		if (componentCount <= expectedCount && componentCount > bestCount)
		{
			bestSeedMask = eroded.clone();
			bestCount = componentCount;
		}
	}

	return bestSeedMask;
}

Mat buildSeedsByPeaks(const Mat& localMask, const Mat& localScore, int expectedCount)
{
	Mat dist;
	distanceTransform(localMask, dist, DIST_L2, 5);
	GaussianBlur(dist, dist, Size(5, 5), 0.0);

	double maxDist = 0.0;
	minMaxLoc(dist, nullptr, &maxDist, nullptr, nullptr);
	if (maxDist <= 0.0)
	{
		return Mat();
	}

	const int peakSize = clampOdd(static_cast<int>(round(maxDist * 1.6)), 5, 21);
	Mat dilated;
	dilate(
		dist,
		dilated,
		getStructuringElement(MORPH_ELLIPSE, Size(peakSize, peakSize)));

	Mat maximaMask;
	compare(dist, dilated, maximaMask, CMP_GE);

	const double thresholdRatio = expectedCount > 1 ? 0.36 : 0.58;
	Mat highMask;
	threshold(dist, highMask, maxDist * thresholdRatio, 255, THRESH_BINARY);
	highMask.convertTo(highMask, CV_8U);

	bitwise_and(maximaMask, highMask, maximaMask);
	bitwise_and(maximaMask, localMask, maximaMask);
	morphologyEx(
		maximaMask,
		maximaMask,
		MORPH_OPEN,
		getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));

	if (countNonZero(maximaMask) == 0)
	{
		return Mat();
	}

	Mat peakLabels;
	Mat peakStats;
	Mat peakCentroids;
	const int peakCount = connectedComponentsWithStats(
		maximaMask,
		peakLabels,
		peakStats,
		peakCentroids,
		8,
		CV_32S);

	struct PeakCandidate
	{
		double strength = 0.0;
		Point center;
	};

	vector<PeakCandidate> peaks;
	for (int label = 1; label < peakCount; ++label)
	{
		Mat currentMask = (peakLabels == label);
		currentMask.convertTo(currentMask, CV_8U, 255);
		if (countNonZero(currentMask) == 0)
		{
			continue;
		}

		double localMax = 0.0;
		Point localPoint;
		minMaxLoc(dist, nullptr, &localMax, nullptr, &localPoint, currentMask);

		PeakCandidate candidate;
		candidate.strength = localMax * 3.0 + mean(localScore, currentMask)[0] * 0.6;
		candidate.center = localPoint;
		peaks.push_back(candidate);
	}

	if (peaks.empty())
	{
		return Mat();
	}

	sort(
		peaks.begin(),
		peaks.end(),
		[](const PeakCandidate& lhs, const PeakCandidate& rhs)
		{
			return lhs.strength > rhs.strength;
		});

	const int keepCount = min(
		max(expectedCount, 1),
		static_cast<int>(peaks.size()));
	const int seedRadius = max(
		2,
		min(6, static_cast<int>(round(maxDist * 0.18))));

	Mat seedMask(localMask.size(), CV_8U, Scalar(0));
	for (int i = 0; i < keepCount; ++i)
	{
		circle(seedMask, peaks[i].center, seedRadius, Scalar(255), FILLED);
	}

	bitwise_and(seedMask, localMask, seedMask);
	return seedMask;
}

Mat buildSeedLabels(
	const Mat& localMask,
	const Mat& localScore,
	int area,
	double medianArea,
	int width,
	int height)
{
	const int expectedCount = estimateSeedCount(localMask, area, medianArea, width, height);

	Mat seedMask = buildSeedsByErosion(localMask, expectedCount);
	if (seedMask.empty())
	{
		seedMask = buildSeedsByPeaks(localMask, localScore, expectedCount);
	}

	if (seedMask.empty() || countNonZero(seedMask) == 0)
	{
		Mat dist;
		distanceTransform(localMask, dist, DIST_L2, 5);
		double maxDist = 0.0;
		Point maxPoint;
		minMaxLoc(dist, nullptr, &maxDist, nullptr, &maxPoint);

		seedMask = Mat(localMask.size(), CV_8U, Scalar(0));
		const int radius = max(2, min(6, static_cast<int>(round(maxDist * 0.22))));
		circle(seedMask, maxPoint, radius, Scalar(255), FILLED);
		bitwise_and(seedMask, localMask, seedMask);
	}

	Mat seedLabels;
	connectedComponents(seedMask, seedLabels, 8, CV_32S);
	return seedLabels;
}

Mat growLabelsInsideMask(const Mat& localMask, const Mat& seedLabels)
{
	Mat grown = seedLabels.clone();
	deque<Point> queue;

	for (int y = 0; y < grown.rows; ++y)
	{
		int* grownRow = grown.ptr<int>(y);
		for (int x = 0; x < grown.cols; ++x)
		{
			if (grownRow[x] > 0)
			{
				queue.push_back(Point(x, y));
			}
		}
	}

	const int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	const int dy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

	while (!queue.empty())
	{
		const Point current = queue.front();
		queue.pop_front();

		const int currentLabel = grown.at<int>(current);
		for (int k = 0; k < 8; ++k)
		{
			const int nx = current.x + dx[k];
			const int ny = current.y + dy[k];
			if (nx < 0 || ny < 0 || nx >= grown.cols || ny >= grown.rows)
			{
				continue;
			}

			if (localMask.at<uchar>(ny, nx) == 0 || grown.at<int>(ny, nx) != 0)
			{
				continue;
			}

			grown.at<int>(ny, nx) = currentLabel;
			queue.push_back(Point(nx, ny));
		}
	}

	return grown;
}

Mat splitConnectedMask(const Mat& cleanedMask, const Mat& scoreMap, double medianArea)
{
	Mat componentLabels;
	Mat stats;
	Mat centroids;
	const int componentCount = connectedComponentsWithStats(
		cleanedMask,
		componentLabels,
		stats,
		centroids,
		8,
		CV_32S);

	Mat splitLabels(cleanedMask.size(), CV_32S, Scalar(0));
	int globalLabel = 1;

	for (int component = 1; component < componentCount; ++component)
	{
		const int area = stats.at<int>(component, CC_STAT_AREA);
		if (area <= 0)
		{
			continue;
		}

		const int left = stats.at<int>(component, CC_STAT_LEFT);
		const int top = stats.at<int>(component, CC_STAT_TOP);
		const int width = stats.at<int>(component, CC_STAT_WIDTH);
		const int height = stats.at<int>(component, CC_STAT_HEIGHT);
		const int pad = 4;

		const Rect roi(
			max(0, left - pad),
			max(0, top - pad),
			min(cleanedMask.cols - max(0, left - pad), width + pad * 2),
			min(cleanedMask.rows - max(0, top - pad), height + pad * 2));

		Mat localMask = (componentLabels(roi) == component);
		localMask.convertTo(localMask, CV_8U, 255);

		Mat localScore = scoreMap(roi).clone();
		bitwise_and(localScore, localMask, localScore);

		Mat seedLabels = buildSeedLabels(localMask, localScore, area, medianArea, width, height);
		Mat grownLabels = growLabelsInsideMask(localMask, seedLabels);

		double localMaxLabel = 0.0;
		minMaxLoc(grownLabels, nullptr, &localMaxLabel, nullptr, nullptr);

		for (int y = 0; y < grownLabels.rows; ++y)
		{
			const int* grownRow = grownLabels.ptr<int>(y);
			int* splitRow = splitLabels.ptr<int>(roi.y + y);
			for (int x = 0; x < grownLabels.cols; ++x)
			{
				if (grownRow[x] > 0)
				{
					splitRow[roi.x + x] = globalLabel + grownRow[x] - 1;
				}
			}
		}

		globalLabel += static_cast<int>(localMaxLabel);
	}

	return splitLabels;
}

Mat colorizeLabels(const Mat& labels)
{
	Mat preview(labels.size(), CV_8UC3, Scalar(32, 32, 32));

	for (int y = 0; y < labels.rows; ++y)
	{
		const int* labelRow = labels.ptr<int>(y);
		Vec3b* previewRow = preview.ptr<Vec3b>(y);
		for (int x = 0; x < labels.cols; ++x)
		{
			const int label = labelRow[x];
			if (label <= 0)
			{
				continue;
			}

			const int r = (label * 47) % 255;
			const int g = (label * 91) % 255;
			const int b = (label * 137) % 255;
			previewRow[x] = Vec3b(
				static_cast<uchar>(b),
				static_cast<uchar>(g),
				static_cast<uchar>(r));
		}
	}

	return preview;
}

Mat renderSplitMask(const Mat& splitLabels)
{
	Mat mask(splitLabels.size(), CV_8U, Scalar(0));
	for (int y = 0; y < splitLabels.rows; ++y)
	{
		const int* labelRow = splitLabels.ptr<int>(y);
		uchar* maskRow = mask.ptr<uchar>(y);
		for (int x = 0; x < splitLabels.cols; ++x)
		{
			if (labelRow[x] > 0)
			{
				maskRow[x] = 255;
			}
		}
	}

	for (int y = 1; y < splitLabels.rows - 1; ++y)
	{
		const int* prevRow = splitLabels.ptr<int>(y - 1);
		const int* labelRow = splitLabels.ptr<int>(y);
		const int* nextRow = splitLabels.ptr<int>(y + 1);
		uchar* maskRow = mask.ptr<uchar>(y);
		for (int x = 1; x < splitLabels.cols - 1; ++x)
		{
			const int currentLabel = labelRow[x];
			if (currentLabel <= 0)
			{
				continue;
			}

			if ((labelRow[x - 1] > 0 && labelRow[x - 1] != currentLabel) ||
				(labelRow[x + 1] > 0 && labelRow[x + 1] != currentLabel) ||
				(prevRow[x] > 0 && prevRow[x] != currentLabel) ||
				(nextRow[x] > 0 && nextRow[x] != currentLabel))
			{
				maskRow[x] = 0;
			}
		}
	}

	return mask;
}

vector<CodexDetection> extractDetections(const Mat& splitLabels, const Size& imageSize)
{
	const double imageArea = static_cast<double>(imageSize.width) * static_cast<double>(imageSize.height);
	const double minArea = max(1200.0, imageArea * 0.00006);

	vector<CodexDetection> detections;
	double maxLabel = 0.0;
	minMaxLoc(splitLabels, nullptr, &maxLabel, nullptr, nullptr);

	for (int label = 1; label <= static_cast<int>(maxLabel); ++label)
	{
		Mat currentMask = (splitLabels == label);
		currentMask.convertTo(currentMask, CV_8U, 255);

		vector<vector<Point>> contours;
		findContours(currentMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		if (contours.empty())
		{
			continue;
		}

		const auto contour = *max_element(
			contours.begin(),
			contours.end(),
			[](const vector<Point>& lhs, const vector<Point>& rhs)
			{
				return contourArea(lhs) < contourArea(rhs);
			});

		const double area = contourArea(contour);
		if (area < minArea)
		{
			continue;
		}

		const Rect box = boundingRect(contour);
		const double fillRatio = area / max(1.0, static_cast<double>(box.area()));
		if (fillRatio < 0.20)
		{
			continue;
		}

		CodexDetection detection;
		detection.area = area;
		detection.perimeter = arcLength(contour, true);
		detection.circularity = detection.perimeter > 0.0
			? (4.0 * CV_PI * area) / (detection.perimeter * detection.perimeter)
			: 0.0;
		detection.box = box;
		detection.contour = contour;
		detections.push_back(detection);
	}

	sort(
		detections.begin(),
		detections.end(),
		[](const CodexDetection& lhs, const CodexDetection& rhs)
		{
			if (lhs.box.y != rhs.box.y)
			{
				return lhs.box.y < rhs.box.y;
			}
			return lhs.box.x < rhs.box.x;
		});

	for (int i = 0; i < static_cast<int>(detections.size()); ++i)
	{
		detections[i].id = i + 1;
	}

	return detections;
}

Mat drawAnnotatedResult(const Mat& image, const vector<CodexDetection>& detections, const string& strategyName)
{
	Mat annotated = image.clone();
	const int minDim = min(image.cols, image.rows);
	const int thickness = max(2, minDim / 900);
	const double fontScale = max(0.8, minDim / 1800.0);
	const int fontThickness = max(2, thickness - 1);

	for (const auto& detection : detections)
	{
		drawContours(annotated, vector<vector<Point>>{ detection.contour }, -1, Scalar(0, 220, 255), thickness);
		rectangle(annotated, detection.box, Scalar(0, 255, 0), thickness);

		const Point labelOrigin(
			detection.box.x,
			max(40, detection.box.y - 12));
		putText(
			annotated,
			to_string(detection.id),
			labelOrigin,
			FONT_HERSHEY_SIMPLEX,
			fontScale,
			Scalar(0, 0, 0),
			fontThickness + 2,
			LINE_AA);
		putText(
			annotated,
			to_string(detection.id),
			labelOrigin,
			FONT_HERSHEY_SIMPLEX,
			fontScale,
			Scalar(255, 255, 255),
			fontThickness,
			LINE_AA);
	}

	const string title = strategyName + " | count = " + to_string(detections.size());
	const int baselineY = max(48, minDim / 18);
	putText(
		annotated,
		title,
		Point(24, baselineY),
		FONT_HERSHEY_SIMPLEX,
		fontScale,
		Scalar(0, 0, 0),
		fontThickness + 4,
		LINE_AA);
	putText(
		annotated,
		title,
		Point(24, baselineY),
		FONT_HERSHEY_SIMPLEX,
		fontScale,
		Scalar(255, 255, 255),
		fontThickness,
		LINE_AA);

	return annotated;
}
}

CodexPipelineResult runCodexPipeline(const Mat& image)
{
	CodexPipelineResult result;
	result.strategyName = "Codex foreground score + seeded split";

	const double imageArea = static_cast<double>(image.cols) * static_cast<double>(image.rows);
	const double minArea = max(1200.0, imageArea * 0.00006);

	result.scoreMap = buildScoreMap(image);
	result.rawMask = buildInitialMask(result.scoreMap);
	result.cleanedMask = cleanMask(result.rawMask, minArea);

	const double medianArea = computeMedianComponentArea(result.cleanedMask, minArea);
	Mat splitLabels = splitConnectedMask(result.cleanedMask, result.scoreMap, medianArea);

	result.markerPreview = colorizeLabels(splitLabels);
	result.splitMask = renderSplitMask(splitLabels);
	result.detections = extractDetections(splitLabels, image.size());
	result.annotated = drawAnnotatedResult(image, result.detections, result.strategyName);

	return result;
}

void saveCodexArtifacts(
	const filesystem::path& outputDir,
	const string& stem,
	const CodexPipelineResult& result)
{
	filesystem::create_directories(outputDir);

	Mat scoreColor;
	applyColorMap(result.scoreMap, scoreColor, COLORMAP_TURBO);

	imwrite((outputDir / (stem + "_codex_score.png")).string(), scoreColor);
	imwrite((outputDir / (stem + "_codex_raw_mask.png")).string(), result.rawMask);
	imwrite((outputDir / (stem + "_codex_clean_mask.png")).string(), result.cleanedMask);
	imwrite((outputDir / (stem + "_codex_split_mask.png")).string(), result.splitMask);
	imwrite((outputDir / (stem + "_codex_markers.png")).string(), result.markerPreview);
	imwrite((outputDir / (stem + "_codex_result.png")).string(), result.annotated);
}
