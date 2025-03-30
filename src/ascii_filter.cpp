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

cv::Mat quantizeLuminance(const cv::Mat &frame) 
{
    // Quantize the luminance values to 10 levels for 10 destinct characters
    cv::Mat quantizedFrame = frame.clone();
    for (int i = 0; i < quantizedFrame.rows; ++i) {
        for (int j = 0; j < quantizedFrame.cols; ++j) {
            uchar pixelValue = quantizedFrame.at<uchar>(i, j);
            int quantizedValue = (pixelValue / 25) * 25;
            quantizedFrame.at<uchar>(i, j) = quantizedValue;
        }
    }
    return quantizedFrame;
}

cv::Mat convertToAscii(cv::Mat& frame)
{
    
}
