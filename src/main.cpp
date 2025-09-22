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
    string videoFile;
    bool saveToFile = false;
    bool useColor = false;
    cv::Scalar color;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--save") {
            saveToFile = true;
        } else if (arg == "--color") {
            useColor = true;
            if (i + 1 < argc) {
                std::string colorStr = argv[++i];
                std::istringstream iss(colorStr);
                char comma;
                int r, g, b;
                if (iss >> r >> comma >> g >> comma >> b && comma == ',') {
                    color = cv::Scalar(b, g, r);  // OpenCV uses BGR
                } else {
                    cerr << "Error: Invalid color format. Use R,G,B." << endl;
                    return -1;
                }
            } else {
                cerr << "Error: --color option requires an argument." << endl;
                return -1;
            }
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [video_file] [--save] [--color R,G,B]" << endl;
            cout << "Options:" << endl;
            cout << "  --save         Save the ASCII video to a file." << endl;
            cout << "  --color R,G,B  Use specified color for ASCII symbols." << endl;
            return 0;
        } else if (videoFile.empty()) {
            videoFile = arg;
        } else {
            cerr << "Unknown argument: " << arg << endl;
            return -1;
        }
    }

    cv::VideoCapture cap;
    if (videoFile.empty()) {
        cout << "No video file provided. Trying to open the default camera..." << endl;
        cap.open(0);
    } else {
        cap.open(videoFile);
    }

    if (!cap.isOpened()) {
        cerr << "Error: Could not open video source." << endl;
        return -1;
    }
    
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps < 1) fps = 30;
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
        string outputFile = baseName + "_ascii.mp4";
        writer.open(outputFile, cv::VideoWriter::fourcc('a', 'v', 'c', '1'), fps, cv::Size(width, height), true);
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
    cv::Mat asciiFrame;

    while (true) {
        cap >> frame;
        startTime = cv::getTickCount(); 

        if (frame.empty()) {
            break;
        }

        if (useColor)
            asciiFrame = convertToAscii(frame, color);
        else
            asciiFrame = convertToAscii(frame);
        frameCount++;
        double duration = (cv::getTickCount() - startTime) / cv::getTickFrequency();
        totalTime += duration;

        if (saveToFile) {
            writer.write(asciiFrame);
            showProgress(frameCount, totalFrames);
        } else {
            if (frameCount % 10 == 0) {
                frames = frameCount / totalTime;
                cout << "Original FPS: " << fps 
                    << " | Processing FPS: " << frames << endl;
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