#ifndef ASCII_FILTER_HPP
#define ASCII_FILTER_HPP

#include <string>
#include <opencv2/opencv.hpp>

const std::string asciiChars = " .icoPO?@■"; // Characters used for ASCII art from dark to light¨

struct EdgeData {
    cv::Mat edges;   // binary edge map
    cv::Mat gradX;   // Sobel X gradients
    cv::Mat gradY;   // Sobel Y gradients
};

cv::Mat downscale(const cv::Mat& frame);
cv::Mat upscale(const cv::Mat& frame);

cv::Mat convertToGrayscale(const cv::Mat& frame);

cv::Mat convertToAscii(cv::Mat& frame, cv::Scalar color = cv::Scalar(-1, -1, -1));

EdgeData detectEdges(const cv::Mat& frame, int kernelSize = 3);

std::pair<cv::Mat, cv::Mat>applyEdgeBasedAscii(const cv::Mat &frame, cv::Scalar color, int kernelSize = 3, bool useOriginalColor = false, const cv::Mat &smallColor = cv::Mat());

void processBlockAscii(const cv::Mat &grayFrame, const cv::Mat &smallColor, cv::Mat&occupancyMask, cv::Mat &asciiArt, int i, int j, bool useOriginalColor, cv::Scalar color);

#endif