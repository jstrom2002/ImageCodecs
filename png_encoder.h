#include <cstdint>
#include <string>
#include <vector>

namespace png_encoder
{
    // Code adapted from: https://github.com/lvandeve/lodepng/
    void saveToFile(std::string filepath, unsigned char* pixels, int w, int h, int d);
}