#include <iostream>     // For input/output
#include <opencv2/opencv.hpp>  // OpenCV main header for video processing
#include "../include/ascii_filter.hpp"
#include <cstdlib>

using namespace std;

// g++ -O2 -o video_player src/main.cpp src/ascii_filter.cpp `pkg-config --cflags --libs opencv4` -Iinclude`

void showProgress(int currentFrame, int totalFrames) {
    int barWidth = 50;  // Width of the progress bar
    float progress = (float)currentFrame / totalFrames;
    int pos = barWidth * progress;

    cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) cout << "#";
        else cout << "-";
    }
    cout << "] " << int(progress * 100.0) << "%\r";  // Display percentage
    cout.flush();
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        cerr << "Usage: " << argv[0] << " <video_file> [--save] [--color]" << std::endl;
        return -1;
    }

    string videoFile = argv[1];
    bool saveToFile = false;
    bool colorOutput = false;
    for (int i = 2; i < argc; ++i){
        if (string(argv[i]) == "--save") {
            saveToFile = true;
        }
        else if (string(argv[i]) == "--color") {
            colorOutput = true;
        }
    }
    cv::VideoCapture cap(videoFile);

    if (!cap.isOpened()) {
        cerr << "Error: Could not open video file." << std::endl;
        return -1;
    }
    
    double fps = cap.get(cv::CAP_PROP_FPS);
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    cv::VideoWriter writer;

    if (saveToFile) {
        size_t lastSlash = videoFile.find_last_of('/');
        size_t lastDot = videoFile.find_last_of('.');

        if (lastDot == std::string::npos || lastDot <= lastSlash) {
            cerr << "Error: Invalid video file name." << endl;
            return -1;
        }

        string baseName = videoFile.substr(lastSlash + 1, lastDot - lastSlash - 1);
        string outputFile = baseName + "_ascii.avi";
        writer.open(outputFile, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height), true);
        if (!writer.isOpened()) {
            cerr << "Error: Could not open output video file for writing." << endl;
            return -1;
        }
        cout << "Saving to " << outputFile << endl;
    }


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

        if (saveToFile) {
            writer.write(asciiFrame);
            showProgress(frameCount, totalFrames);
        } else {
            if (frameCount % 10 == 0) {
                frames = frameCount / totalTime;
                cout << "FPS: " << frames << endl;
            }
            cv::imshow("ASCII Video", asciiFrame);
            if (cv::waitKey(1000 / fps) >= 0) {
                break;
            }
        }

    }
    cap.release();
    writer.release();
    cv::destroyAllWindows();
}