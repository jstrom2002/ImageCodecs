#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>

namespace PPM
{
    class PPM
    {
    public:
        PPM() = default;
        PPM(const std::string& filepath);
        PPM(const uint8_t* buffer, const int h, const int w, const int max, const std::string magic);
        PPM(const PPM& ppm);
        ~PPM();
        PPM& operator = (const PPM& ppm);
        bool operator == (const PPM& ppm) const;
        int read(const std::string& filepath);
        int write(const std::string& filepath) const;
        void load(const uint8_t* buffer, const int h, const int w, const int max, const std::string magic);
        std::string getMagic() const;
        std::string getFilepath() const;
        int getH() const;
        int getW() const;
        int getMax() const;
        uint8_t* getImageHandler() const;
        void setBinary(const bool isBinary);
    private:
        std::string mMagic, mFilepath;
        int mH, mW, mMax;
        uint8_t* mBuffer = nullptr;
    };
}