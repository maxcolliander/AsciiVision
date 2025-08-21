#include "ascii_filter.hpp"

cv::Mat downscale(const cv::Mat &frame)
{
    // Downscale the image to a smaller size divide by 8
    int newWidth = frame.cols / 8;
    int newHeight = frame.rows / 8;
    cv::Size newSize(newWidth, newHeight);
    cv::Mat downscaledFrame;
    cv::resize(frame, downscaledFrame, newSize, 0, 0, cv::INTER_LINEAR);
    return downscaledFrame;
}

cv::Mat upscale(const cv::Mat &frame)
{
    int newWidth = frame.cols * 8;
    int newHeight = frame.rows * 8;
    cv::Size newSize(newWidth, newHeight);
    cv::Mat upscaledFrame;
    cv::resize(frame, upscaledFrame, newSize, 0, 0, cv::INTER_LINEAR);
    return upscaledFrame;
}

cv::Mat convertToGrayscale(const cv::Mat &frame)
{
    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
    return grayFrame;
}

cv::Mat convertToAscii(cv::Mat &frame, cv::Scalar color)
{
    cv::Mat grayFrame = convertToGrayscale(frame);

    auto [edgeAsciiArt, occupancyMask] = applyEdgeBasedAscii(frame, color, 3);

    cv::parallel_for_(cv::Range(0, grayFrame.rows), [&](const cv::Range& range) {
        for (int i = range.start; i < range.end; ++i) {
            for (int j = 0; j < grayFrame.cols; ++j) {
                processBlockAscii(grayFrame, occupancyMask, edgeAsciiArt, i, j, color);
            }
        }
    });
    return edgeAsciiArt;
}

EdgeData detectEdges(const cv::Mat &frame, int kernelSize)
{
    cv::Mat blurred, edges, gradX, gradY;

    // Reduce noise with gaussian blur
    cv::GaussianBlur(frame, blurred, cv::Size(kernelSize, kernelSize), 0);

    // Canny edge detection edges -> binary edge map
    cv::Canny(blurred, edges, 100, 200);

    // Sobel edge detection for gradients
    cv::Sobel(blurred, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(blurred, gradY, CV_32F, 0, 1, 3);

    return {edges, gradX, gradY};
}

void processBlockAscii(const cv::Mat &grayFrame, cv::Mat&occupancyMask, cv::Mat &asciiArt, int i, int j, cv::Scalar color)
{
    if (i % 8 != 0 || j % 8 != 0) {
        return;
    }
    if (occupancyMask.at<uchar>(i, j) == 255) {
        return;
    } 

    int blockSum = 0;
    int pixelCount = 0;
    const uchar* rowPtr;
    uchar* maskPtr;
    for (int y = i; y < std::min(i + 8, grayFrame.rows); ++y) {
        rowPtr = grayFrame.ptr<uchar>(y);
        maskPtr = occupancyMask.ptr<uchar>(y);
        for (int x = j; x < std::min(j + 8, grayFrame.cols); ++x) {
            blockSum += rowPtr[x];
            pixelCount++;
            maskPtr[x] = 255;
        }
    }
    int avgLuminance = blockSum / pixelCount;
    int asciiIndex = avgLuminance * (asciiChars.length() - 1) / 255;
    char asciiChar = asciiChars[asciiIndex];

    cv::putText(asciiArt, std::string(1, asciiChar), cv::Point(j, i + 8), cv::FONT_HERSHEY_PLAIN, 0.5, color, 1, cv::LINE_AA);
}

std::pair<cv::Mat, cv::Mat> applyEdgeBasedAscii(const cv::Mat &frame, cv::Scalar color, int kernelSize) {

    // Convert to grayscale and downscale for cheaper edge detection
    cv::Mat grayFrame = convertToGrayscale(frame);
    cv::Mat smallGray = downscale(grayFrame);

    // Detect edges using Sobel and Canny methods
    EdgeData edgeData = detectEdges(smallGray, kernelSize);

    // Create an empty ASCII art image
    cv::Mat edgeAsciiArt(frame.size(), CV_8UC3, cv::Scalar(0, 0, 0));

    // Create an occupancy mask to track processed blocks
    cv::Mat occupancyMask(frame.size(), CV_8UC1, cv::Scalar(0));

    float scaleX = static_cast<float>(frame.cols) / smallGray.cols;
    float scaleY = static_cast<float>(frame.rows) / smallGray.rows;

    int cellSize = 8;

    for (int i = 0; i < smallGray.rows; i++) {
        for (int j = 0; j < smallGray.cols; j++) {
            if (edgeData.edges.at<uchar>(i, j) != 0) {
                float gx = edgeData.gradX.at<float>(i, j);
                float gy = edgeData.gradY.at<float>(i, j);
                char edgeChar;
                float absGx = std::abs(gx), absGy = std::abs(gy);
                if (absGx > absGy * 2) edgeChar = '-';
                else if (absGy > absGx * 2) edgeChar = '|';
                else if ((gx > 0 && gy > 0) || (gx < 0 && gy < 0)) edgeChar = '/';
                else edgeChar = '\\';

                int fullX = static_cast<int>(j * scaleX);
                int fullY = static_cast<int>(i * scaleY);

                fullX = (fullX / cellSize) * cellSize;
                fullY = (fullY / cellSize) * cellSize;

                cv::putText(edgeAsciiArt, std::string(1, edgeChar), cv::Point(fullX, fullY + cellSize), cv::FONT_HERSHEY_PLAIN, 0.5, color, 1, cv::LINE_AA);

                cv::rectangle(occupancyMask, cv::Rect(fullX, fullY, cellSize, cellSize), cv::Scalar(255), cv::FILLED);
            }
        }
    }

    return {edgeAsciiArt, occupancyMask};
}