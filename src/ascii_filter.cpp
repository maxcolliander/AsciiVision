#include "ascii_filter.hpp"

cv::Mat downscale(const cv::Mat &frame)
{
    // Downscale the image to a smaller size divide by 8
    int newWidth = frame.cols / 8;
    int newHeight = frame.rows / 8;
    cv::Size newSize(newWidth, newHeight);
    cv::Mat downscaledFrame;
    cv::resize(frame, downscaledFrame, newSize, 0, 0, cv::INTER_AREA);
    return downscaledFrame;
}

cv::Mat upscale(const cv::Mat &frame)
{
    int newWidth = frame.cols * 8;
    int newHeight = frame.rows * 8;
    cv::Size newSize(newWidth, newHeight);
    cv::Mat upscaledFrame;
    cv::resize(frame, upscaledFrame, newSize, 0, 0, cv::INTER_AREA);
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
    const int cellSize = 8;
    bool useOriginalColor = (color[0] < 0);

    cv::Mat smallColor;
    if (useOriginalColor) {
        int cellSize = 8;
        // Precompute block colors
        cv::resize(frame, smallColor, cv::Size(frame.cols / cellSize, frame.rows / cellSize), 0, 0, cv::INTER_AREA);
    }

    auto [edgeAsciiArt, occupancyMask] = applyEdgeBasedAscii(frame, color, 3, useOriginalColor, smallColor);

    int rows = frame.rows;
    int cols = frame.cols;
    cv::parallel_for_(cv::Range(0, rows / cellSize), [&](const cv::Range& range) {
        for (int bi = range.start; bi < range.end; ++bi) {
            for (int bj = 0; bj < cols / cellSize; ++bj) {
                int i = bi * cellSize;
                int j = bj * cellSize;
                processBlockAscii(frame, smallColor, occupancyMask, edgeAsciiArt, i, j, useOriginalColor, color);
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

    // Apply median blur to reduce noise further
    cv::medianBlur(blurred, blurred, 3);

    // Canny edge detection edges -> binary edge map
    double medianVal = cv::mean(blurred)[0];
    double lower = std::max(0.0, 0.7 * medianVal);
    double upper = std::min(255.0, 1.3 * medianVal);
    cv::Canny(blurred, edges, lower, upper);

    // Sobel edge detection for gradients
    cv::Sobel(blurred, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(blurred, gradY, CV_32F, 0, 1, 3);

    return {edges, gradX, gradY};
}

void processBlockAscii(const cv::Mat &frame, const cv::Mat &smallColor, cv::Mat&occupancyMask, cv::Mat &asciiArt, int i, int j, bool useOriginalColor, cv::Scalar color)
{
    const int cellSize = 8;
    if (occupancyMask.at<uchar>(i, j) == 255) return;

    int blockSum = 0;
    int pixelCount = 0;

    // Compute average luminance for the block
    for (int y = i; y < std::min(i + cellSize, frame.rows); ++y) {
        const cv::Vec3b* rowPtr = frame.ptr<cv::Vec3b>(y);
        uchar* maskPtr = occupancyMask.ptr<uchar>(y);
        for (int x = j; x < std::min(j + cellSize, frame.cols); ++x) {
            const cv::Vec3b &px = rowPtr[x];
            blockSum += static_cast<int>(0.2126*px[2] + 0.7152*px[1] + 0.0722*px[0]);
            pixelCount++;
            maskPtr[x] = 255;
        }
    }

    int avgLuminance = blockSum / pixelCount;
    char asciiChar = asciiChars[avgLuminance * (asciiChars.size() - 1) / 255];

    // Get block color
    cv::Scalar colorToUse = color;
    if (useOriginalColor && !smallColor.empty()) {
        int smallI = i / cellSize;
        int smallJ = j / cellSize;
        cv::Vec3b avgColorVec = smallColor.at<cv::Vec3b>(smallI, smallJ);
        colorToUse = cv::Scalar(avgColorVec[0], avgColorVec[1], avgColorVec[2]);
    }

    cv::putText(asciiArt, std::string(1, asciiChar), cv::Point(j, i + cellSize), cv::FONT_HERSHEY_PLAIN, 0.5, colorToUse, 1, cv::LINE_AA);
}

std::pair<cv::Mat, cv::Mat> applyEdgeBasedAscii(const cv::Mat &frame, cv::Scalar color, int kernelSize, bool useOriginalColor, const cv::Mat &smallColor) 
{
    const int cellSize = 8;
    cv::Mat grayFrame = convertToGrayscale(frame);
    cv::Mat smallGray = downscale(grayFrame);
    EdgeData edgeData = detectEdges(smallGray, kernelSize);

    cv::Mat edgeAsciiArt(frame.size(), CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat occupancyMask(frame.size(), CV_8UC1, cv::Scalar(0));

    float scaleX = float(frame.cols) / smallGray.cols;
    float scaleY = float(frame.rows) / smallGray.rows;

    cv::parallel_for_(cv::Range(0, smallGray.rows), [&](const cv::Range &range) {
        for (int i = range.start; i < range.end; ++i) {
            for (int j = 0; j < smallGray.cols; ++j) {
                if (edgeData.edges.at<uchar>(i,j) == 0) continue;

                float gx = edgeData.gradX.at<float>(i,j);
                float gy = edgeData.gradY.at<float>(i,j);
                float magnitude = std::sqrt(gx*gx + gy*gy);
                float threshold = 30.0f * (frame.cols / float(smallGray.cols));
                if (magnitude < threshold) continue;

                char edgeChar;
                float angle = std::atan2(gy, gx) * 180.0f / CV_PI;

                if (std::abs(angle) < 22.5 || std::abs(angle) > 157.5) {
                    edgeChar = '|';
                }
                else if (std::abs(angle) > 67.5 && std::abs(angle) < 112.5) {
                    edgeChar = '-';
                }
                else if ((angle > 0 && angle < 67.5) || (angle < -112.5 && angle > -180)) {
                    edgeChar = '/';
                }
                else {
                    edgeChar = '\\';
                }
                int fullX = (int)(j*scaleX);
                int fullY = (int)(i*scaleY);
                fullX = (fullX/cellSize)*cellSize;
                fullY = (fullY/cellSize)*cellSize;

                cv::Scalar colorToUse = color;
                if (useOriginalColor && !smallColor.empty()) {
                    int smallI = fullY / cellSize;
                    int smallJ = fullX / cellSize;
                    cv::Vec3b avgColorVec = smallColor.at<cv::Vec3b>(smallI, smallJ);
                    colorToUse = cv::Scalar(avgColorVec[0], avgColorVec[1], avgColorVec[2]);
                }

                cv::putText(edgeAsciiArt, std::string(1, edgeChar), cv::Point(fullX, fullY+cellSize),
                            cv::FONT_HERSHEY_PLAIN, 0.5, colorToUse, 1, cv::LINE_AA);
                cv::rectangle(occupancyMask, cv::Rect(fullX, fullY, cellSize, cellSize), cv::Scalar(255), cv::FILLED);
            }
        }
    });

    return {edgeAsciiArt, occupancyMask};
}