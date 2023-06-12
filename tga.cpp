#include "tga.h"
#include <stdio.h>
#include <exception>
#include <fstream>
#include <vector>

namespace TGA
{
    std::vector<uint8_t> loadTga(const char* FilePath, int& w, int& h)
    {
        std::fstream hFile(FilePath, std::ios::in | std::ios::binary);
        if (!hFile.is_open()) 
        { 
            throw std::invalid_argument("File Not Found."); 
        }

        std::uint8_t Header[18] = { 0 };
        std::vector<std::uint8_t> ImageData;
        static std::uint8_t DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
        static std::uint8_t IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

        hFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));
        if (!std::memcmp(DeCompressed, &Header, sizeof(DeCompressed)))
        {
            int BitsPerPixel = Header[16];
            int width = Header[13] * 256 + Header[12];
            int height = Header[15] * 256 + Header[14];
            w = width;
            h = height;
            int size = ((width * BitsPerPixel + 31) / 32) * 4 * height;

            if (BitsPerPixel != 24)
            {
                hFile.close();
                throw std::invalid_argument("Invalid File Format. Required: 24 Bit Image.");
            }

            ImageData.resize(size);
            bool ImageCompressed = false;
            hFile.read(reinterpret_cast<char*>(ImageData.data()), size);
        }
        else if (!std::memcmp(IsCompressed, &Header, sizeof(IsCompressed)))
        {
            throw new std::exception("Error! Cannot read compressed .tga files");
        }
        else
        {
            hFile.close();
            throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit TGA File.");
        }

        hFile.close();
        return ImageData;
    }

	void saveTga(const char* filename, int width, int height, int dataChannels, int fileChannels, unsigned char* dataBGRA)
	{
		FILE* fp = NULL;
		fp = fopen(filename, "wb");
		if (fp == NULL) 
            return;

		unsigned char header[18] = 
        {
            0,0,2,0,0,0,0,0,0,0,0,0, 
            (unsigned char)(width % 256), 
            (unsigned char)(width / 256), 
            (unsigned char)(height % 256), 
            (unsigned char)(height / 256), 
            (unsigned char)(dataChannels * 8), 
            0x20 
        };
		fwrite(&header, 18, 1, fp);

		for (int i = 0; i < width * height; i++)
		{
			for (int b = 0; b < fileChannels; b++)
			{
				fputc(dataBGRA[(i * dataChannels) + (b % dataChannels)], fp);
			}
		}
		fclose(fp);
	}
}