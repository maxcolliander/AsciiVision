#include <iostream>     // For input/output
#include <opencv2/opencv.hpp>  // OpenCV main header for video processing
#include "../include/ascii_filter.hpp"

using namespace std;

// g++ -o video_player main.cpp `pkg-config --cflags --libs opencv4`

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return -1;
    }
    string videoFile = argv[1];
    cv::VideoCapture cap(videoFile);  // Open the video file

    if (!cap.isOpened()) {
        cerr << "Error: Could not open video file." << std::endl;
        return -1;
    }
    
    double fps = cap.get(cv::CAP_PROP_FPS);
    cout << "FPS: " << fps << endl;

    cv::Mat frame;

    while (true) {
        cap >> frame; 

        if (frame.empty()) {
            break;
        }

        cv::Mat smallFrame = downscale(frame);
        cv::Mat resizedFrame = upscale(smallFrame);
        cv::Mat grayFrame = convertToGrayscale(resizedFrame);
        cv::Mat quantizedFrame = quantizeLuminance(grayFrame);
        cv::Mat asciiFrame = convertToAscii(quantizedFrame);

        cv::imshow("ASCII Video", asciiFrame);
        

        if (cv::waitKey(1000 / fps) >= 0) {
            break;
        }

    }
    cap.release();
    cv::destroyAllWindows();
}