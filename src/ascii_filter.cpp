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

cv::Mat convertToAscii(cv::Mat &frame)
{
    cv::Mat grayFrame = convertToGrayscale(frame);

    auto [edgeAsciiArt, occupancyMask] = applyEdgeBasedAscii(frame, 3);

    cv::parallel_for_(cv::Range(0, grayFrame.rows), [&](const cv::Range& range) {
        for (int i = range.start; i < range.end; ++i) {
            for (int j = 0; j < grayFrame.cols; ++j) {
                processBlockAscii(grayFrame, occupancyMask, edgeAsciiArt, i, j);
            }
        }
    });
    return edgeAsciiArt;
}

void processBlockAscii(const cv::Mat &grayFrame, cv::Mat&occupancyMask, cv::Mat &asciiArt, int i, int j)
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

    cv::putText(asciiArt, std::string(1, asciiChar), cv::Point(j, i + 8), cv::FONT_HERSHEY_PLAIN, 0.5, cv::Scalar(255, 0, 255), 1, cv::LINE_AA);
}

cv::Mat applyCanny(const cv::Mat &frame, int kernelSize)
{
    cv::Mat blurredImage, edges;
    cv::GaussianBlur(frame, blurredImage, cv::Size(kernelSize, kernelSize), 0);
    cv::Canny(blurredImage, edges, 100, 200);

    return edges;
}

std::pair<cv::Mat, cv::Mat>applyEdgeBasedAscii(const cv::Mat &grayFrame, int kernelSize)
{
    cv::Mat edges = applyCanny(grayFrame, kernelSize);

    cv::Mat edgeAsciiArt = cv::Mat::zeros(edges.size(), CV_8UC3);

    cv::Mat occupancyMask = cv::Mat::zeros(edges.size(), CV_8UC1);

    uchar* edgePtr;
    uchar* maskPtr;
    for (int i = 0; i < edges.rows; i += 8) {
        for (int j = 0; j < edges.cols; j += 8) {
            int edgePixelCount = 0;
            int totalPixels = 0;
            double avgAngle = 0.0;
            int gx = 0;
            int gy = 0;
            for (int y = i; y < std::min(i + 8, edges.rows); ++y) {
                edgePtr = edges.ptr<uchar>(y);
                maskPtr = occupancyMask.ptr<uchar>(y);
                for (int x = j; x < std::min(j + 8, edges.cols); ++x) {
                    if (edgePtr[x] == 255) { 
                        edgePixelCount++;
                        totalPixels++;


                        if (x > 0 && x < edges.cols - 1 && y > 0 && y < edges.rows - 1) {
                            gx = edgePtr[x + 1] - edgePtr[x - 1];
                            gy = edgePtr[x] - edgePtr[x];
                        }

                        double angle = std::atan2(gy, gx) * 180.0 / CV_PI;
                        if (angle < 0) {
                            angle += 180.0;
                        }
                        avgAngle += angle;
                    }
                }
            }


            if (totalPixels == 0) {
                continue; 
            }

            avgAngle /= totalPixels;

            char edgeChar;
            if (avgAngle >= 80 && avgAngle < 100) {
                edgeChar = '|';  
            }
            else if (avgAngle >= 30 && avgAngle < 60) {
                edgeChar = '/';  
            }
            else if (avgAngle >= 120 && avgAngle < 150) {
                edgeChar = '\\';  
            }
            else if ((avgAngle >= 0 && avgAngle < 10) || (avgAngle >= 170 && avgAngle < 180)) {
                edgeChar = '-';  
            }
            else {
                continue;
            }

            cv::putText(edgeAsciiArt, std::string(1, edgeChar), cv::Point(j, i + 8), cv::FONT_HERSHEY_PLAIN, 0.5, cv::Scalar(255, 0, 255), 1, cv::LINE_AA);
            for (int y = i; y < std::min(i + 8, edges.rows); ++y) {
                maskPtr = occupancyMask.ptr<uchar>(y);
                for (int x = j; x < std::min(j + 8, edges.cols); ++x) {
                    maskPtr[x] = 255;
                }
            }
        }
    }

    return std::make_pair(edgeAsciiArt, occupancyMask);
}
