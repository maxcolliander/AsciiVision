#include <iostream>     // For input/output
#include <opencv2/opencv.hpp>  // OpenCV main header for video processing
#include "../include/ascii_filter.hpp"

using namespace std;

// g++ -O2 -o video_player src/main.cpp src/ascii_filter.cpp `pkg-config --cflags --libs opencv4` -Iinclude`

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

    int frameCount = 0;
    double totalTime = 0.0;
    double startTime = 0.0;
    double frames = 0.0;


    while (true) {
        cap >> frame;
        startTime = cv::getTickCount(); 

        if (frame.empty()) {
            break;
        }

        cv::Mat asciiFrame = convertToAscii(frame);
        frameCount++;
        double duration = (cv::getTickCount() - startTime) / cv::getTickFrequency();
        totalTime += duration;
        if (frameCount % 10 == 0) {
            frames = frameCount / totalTime;
            cout << "FPS: " << frames << endl;
        }
        
        cv::imshow("ASCII Video", asciiFrame);
        if (cv::waitKey(1000 / fps) >= 0) {
            break;
        }

    }
    cap.release();
    cv::destroyAllWindows();
}