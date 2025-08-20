#ifndef ASCII_FILTER_HPP
#define ASCII_FILTER_HPP

#include <string>
#include <opencv2/opencv.hpp>

const std::string asciiChars = " .icoPO?@■"; // Characters used for ASCII art from dark to light¨

cv::Mat downscale(const cv::Mat& frame);

cv::Mat upscale(const cv::Mat& frame);

cv::Mat convertToGrayscale(const cv::Mat& frame);

cv::Mat convertToAscii(cv::Mat& frame, bool useSameColor = false);

cv::Mat applyCanny(const cv::Mat& frame, int kernelSize = 3);

std::pair<cv::Mat, cv::Mat>applyEdgeBasedAscii(const cv::Mat &frame, int kernelSize = 3);

void processBlockAscii(const cv::Mat &grayFrame, cv::Mat&occupancyMask, cv::Mat &asciiArt, int i, int j);

#endif