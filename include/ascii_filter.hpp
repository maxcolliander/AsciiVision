#ifndef ASCII_FILTER_HPP
#define ASCII_FILTER_HPP

#include <string>
#include <opencv2/opencv.hpp>

const char asciiChars[10] = {' ', '.', ':', '+', 'o', 'O', 'M', 'W', '@'}; // Characters used for ASCII art from dark to light

cv::Mat downscale(const cv::Mat& frame);

cv::Mat upscale(const cv::Mat& frame);

cv::Mat convertToGrayscale(const cv::Mat& frame);

cv::Mat quantizeLuminance(const cv::Mat& frame);

cv::Mat convertToAscii(cv::Mat& frame);

cv::Mat detectEdges(const cv::Mat& frame);

cv::Mat applyGaussianBlur(const cv::Mat& frame);

cv::Mat applyCanny(const cv::Mat& frame);

cv::Mat applyEdgeBasedAscii(const cv::Mat& frame);

cv::Mat applyAsciiFilter(const cv::Mat& frame);




#endif