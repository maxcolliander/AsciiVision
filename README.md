# AsciiVision
AsciiVision is a video filter that converts video frames into ASCII art. It supports processing video files and includes features like edge detection and ASCII-based rendering.

## Demo 
![AsciiVision Demo](media/asciigif.gif)

---
## Features
- Converts video frames into ASCII art.
- Supports edge-based ASCII rendering using Canny and Sobel edge detection.
- Parallelized processing for improved performance.
- Modular design for easy extension.
- Option to save processed videos to disk.

---
## Installation
Follow these steps to set up AsciiVision on your system:

1. **Update your system and install dependencies**:
   ```sh
   sudo apt update
   sudo apt install cmake libopencv-dev
   ```

2. **Clone the repository**:
   ```sh
   git clone <repository-url>
   cd AsciiVision
   ```

3. **Build the project**:
   ```sh
   mkdir build
   cd build
   cmake ..
   make
   ```
4. **Prepare the environment**:
   - Create a `data` directory to store video files for testing:
     ```sh
     mkdir ../data
     ```

5. **Configure Intellisense (Optional)**:
   - Add the following line to your `c_cpp_properties.json` file for proper OpenCV support:
     ```json
     "/usr/include/opencv4/**"
     ```

---

## Usage
### Running the Program
To process a video file, use the following command:
```sh
./AsciiVision <video_file> [--save] [--color R,G,B]
```
- `<video_file>`: Path to the input video file.
- `--save`: (Optional) Save the processed video to disk.
- `--color R,G,B`: (Optional) Color all ASCII symbols with the specified RGB color(e.g., `--color 255,0,0` for red).
- `--help`: Display a help message.

### Example
```sh
./AsciiVision ../data/sample.mp4 --save --color 0,255,0
```

### Directory Structure
- **`data/`**: Stores video files for testing.
- **`include/`**: Contains header files.
- **`src/`**: Contains source code files.

---

## TODO
Here are some planned improvements and features for AsciiVision:
- [ ] Add support for webcam usage and live footage processing.
- [ ] Modularize the ASCII processing pipeline for better maintainability.
- [ ] Implement color correction for better visual output --saturation.
- [ ] Add braille option --braille
- [ ] Add settings as a input to -> convertToAscii function

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

