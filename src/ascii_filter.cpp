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
        int blocksY = frame.rows / cellSize;
        int blocksX = frame.cols / cellSize;
        
        // Heuristic: use resize for landscape (wide) frames, integral images for portrait (tall) frames
        // Resize is super-optimized for wide frames; integral images are faster for tall frames
        if (frame.cols >= frame.rows) {
            // Landscape: use fast resize
            cv::resize(frame, smallColor, cv::Size(blocksX, blocksY), 0, 0, cv::INTER_AREA);
        } else {
            // Portrait: use integral images for better performance
            smallColor.create(blocksY, blocksX, CV_8UC3);

            std::vector<cv::Mat> bgr;
            cv::split(frame, bgr);

            cv::Mat ib, ig, ir;
            cv::integral(bgr[0], ib, CV_64F);
            cv::integral(bgr[1], ig, CV_64F);
            cv::integral(bgr[2], ir, CV_64F);

            auto blockMean = [&](const cv::Mat& I, int y0, int x0, int h, int w) -> double {
                const double* p0 = I.ptr<double>(y0);
                const double* p1 = I.ptr<double>(y0 + h);
                double sum = p1[x0 + w] - p1[x0] - p0[x0 + w] + p0[x0];
                return sum / (h * w);
            };

            int stripesColor = std::max(1, std::min(blocksY, cv::getNumThreads() * 2));
            cv::parallel_for_(cv::Range(0, blocksY), [&](const cv::Range& range){
                for (int by = range.start; by < range.end; ++by) {
                    cv::Vec3b* dst = smallColor.ptr<cv::Vec3b>(by);
                    int y0 = by * cellSize;
                    for (int bx = 0; bx < blocksX; ++bx) {
                        int x0 = bx * cellSize;
                        double mb = blockMean(ib, y0, x0, cellSize, cellSize);
                        double mg = blockMean(ig, y0, x0, cellSize, cellSize);
                        double mr = blockMean(ir, y0, x0, cellSize, cellSize);
                        dst[bx] = cv::Vec3b(
                            static_cast<uchar>(std::clamp(mb, 0.0, 255.0)),
                            static_cast<uchar>(std::clamp(mg, 0.0, 255.0)),
                            static_cast<uchar>(std::clamp(mr, 0.0, 255.0))
                        );
                    }
                }
            }, stripesColor);
        }
    }

    auto [edgeAsciiArt, occupancyMask] = applyEdgeBasedAscii(frame, color, 3, useOriginalColor, smallColor);

    int rows = frame.rows;
    int cols = frame.cols;
    int blockRows = rows / cellSize;
    // Adaptive stripes: cap number of tasks to about 2x OpenCV thread count to reduce scheduling overhead on tall frames
    int stripesBlocks = std::max(1, std::min(blockRows, cv::getNumThreads() * 2));
    cv::parallel_for_(cv::Range(0, blockRows), [&](const cv::Range& range) {
        for (int bi = range.start; bi < range.end; ++bi) {
            for (int bj = 0; bj < cols / cellSize; ++bj) {
                int i = bi * cellSize;
                int j = bj * cellSize;
                processBlockAscii(frame, smallColor, occupancyMask, edgeAsciiArt, i, j, useOriginalColor, color);
            }
        }
    }, stripesBlocks);
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

// Bitmap font: 8x8 monospace glyphs for ASCII characters " .icoPO?@■" and block char (127)
// Each character is stored as 8 rows of 8 bits (1 = pixel on, 0 = pixel off)
static const std::map<char, std::array<uint8_t, 8>> g_fontBitmap = {
    {' ', {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    {'.', {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00}}},
    {'i', {{0x00, 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x00}}},
    {'c', {{0x00, 0x00, 0x1C, 0x20, 0x20, 0x20, 0x1C, 0x00}}},
    {'o', {{0x00, 0x00, 0x18, 0x24, 0x24, 0x24, 0x18, 0x00}}},
    {'P', {{0x00, 0x38, 0x24, 0x38, 0x20, 0x20, 0x3E, 0x00}}},
    {'O', {{0x00, 0x18, 0x24, 0x24, 0x24, 0x24, 0x18, 0x00}}},
    {'?', {{0x00, 0x38, 0x44, 0x04, 0x08, 0x00, 0x08, 0x00}}},
    {'@', {{0x00, 0x18, 0x24, 0x2A, 0x2A, 0x20, 0x1C, 0x00}}},
    {'#', {{0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x00}}},
    // Edge detection characters
    {'|', {{0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00}}}, // Vertical line
    {'-', {{0x00, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x00}}}, // Horizontal line
    {'/', {{0x00, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x00}}}, // Diagonal /
    {'\\', {{0x00, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x00}}}, // Diagonal 
};

// Fast blitter: draw an 8x8 character bitmap at (baseX, baseY) with given color
void blitCharacter(cv::Mat &dst, int baseX, int baseY, char ch, cv::Vec3b color) {
    auto it = g_fontBitmap.find(ch);
    if (it == g_fontBitmap.end()) return; // Character not in font

    const auto& glyph = it->second;
    int maxY = std::min(baseY + 8, dst.rows);
    int maxX = std::min(baseX + 8, dst.cols);

    for (int py = baseY; py < maxY; ++py) {
        cv::Vec3b* row = dst.ptr<cv::Vec3b>(py);
        int glyphRow = py - baseY;
        uint8_t rowBits = glyph[glyphRow];

        for (int px = baseX; px < maxX; ++px) {
            int bitIdx = 7 - (px - baseX);
            if ((rowBits >> bitIdx) & 1) {
                row[px] = color;
            }
        }
    }
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

    blitCharacter(asciiArt, j, i, asciiChar, cv::Vec3b(colorToUse[0], colorToUse[1], colorToUse[2]));
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

    int stripesEdges = std::max(1, std::min(smallGray.rows, cv::getNumThreads() * 2));
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

                // Use fast bitmap blitter instead of putText
                blitCharacter(edgeAsciiArt, fullX, fullY, edgeChar, cv::Vec3b(colorToUse[0], colorToUse[1], colorToUse[2]));
                cv::rectangle(occupancyMask, cv::Rect(fullX, fullY, cellSize, cellSize), cv::Scalar(255), cv::FILLED);
            }
        }
    }, stripesEdges);

    return {edgeAsciiArt, occupancyMask};
}