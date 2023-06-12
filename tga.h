#pragma once
#include <cstdint>
#include <vector>

namespace TGA
{
	// Load data from .tga file (only 24-bit images will be loaded)
	std::vector<uint8_t> loadTga(const char* FilePath);

	// Save data to 24-bit RGB .tga file
	void saveTga(const char* filename, int width, int height, int dataChannels, int fileChannels, unsigned char* dataBGRA);
}