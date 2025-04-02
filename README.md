# AsciiVision
Ascii Video Filter

```sh
sudo apt update
sudo apt install cmake // Could require vscode restart, scan for kits if requested
sudo apt install libopencv-dev
mkdir build
mkdir data
cd build
cmake ..
make
```

## Intellisense
- Make sure to add follow line to c_cpp_properties.json
```
"/usr/include/opencv4/**"
```

## Usage
- Data is used to store video files to be used for testing
- include contains header
- src contains cpp files

## TODO
- Parallelize applyEdgeBasedAscii function
- Improve applyEdgeBasedAscii / Edge detection maybe combine sobel and canny for better edge detection
- Fix image stuttering
- Support webcamera usage and live footage
- Add color method, ascii symbols take on color from footage
- Add texture packs, cyberpunk, matrix and more
- Add a possibility to do --help from terminal to get available features
- Create ascii function to handle all the middle steps, --> more modular
