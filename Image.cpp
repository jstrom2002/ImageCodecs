#include "Image.h"
#include "codecs.h"
#include <filesystem>

namespace ImageCodecs
{
	void Image::read(std::string filepath)
	{
		auto ext = std::filesystem::path(filepath).extension().string();
		for (auto& c : ext)
			c = std::tolower(c);
		if (ext == ".bmp")
			readBmp(filepath,pixels_,w_,h_,d_);
		else if (ext == ".dds")
			readDds(filepath, pixels_, w_, h_, d_);
		else if (ext == ".gif")
			readGif(filepath, pixels_, w_, h_, d_);
		else if (ext == ".hdr")
			readHdr(filepath, pixels_, w_, h_, d_);
		else if (ext == ".jpg" || ext == ".jpeg")
			readJpg(filepath, pixels_, w_, h_, d_);
		else if (ext == ".png")
			readPng(filepath, pixels_, w_, h_, d_);
		else if (ext == ".ppm")
			readPpm(filepath, pixels_, w_, h_, d_);
		else if (ext == ".tga")
			readTga(filepath, pixels_, w_, h_, d_);
		else if (ext == ".tiff" || ext == ".tif")
			readTiff(filepath, pixels_, w_, h_, d_);
		else if (ext == ".webp")
			readWebp(filepath, pixels_, w_, h_, d_);
		else
		{
			throw std::invalid_argument("Cannot parse filetype");
		}
	}

	void Image::write(std::string filepath)
	{
		auto ext = std::filesystem::path(filepath).extension().string();
		for (auto& c : ext)
			c = std::tolower(c);
		if (ext == ".bmp")
			writeBmp(filepath, pixels_, w_, h_, d_);
		else if (ext == ".dds")
			writeDds(filepath, pixels_, w_, h_, d_);
		else if (ext == ".gif")
			writeGif(filepath, pixels_, w_, h_, d_);
		else if (ext == ".hdr")
			writeHdr(filepath, pixels_, w_, h_, d_);
		else if (ext == ".jpg" || ext == ".jpeg")
			writeJpg(filepath, pixels_, w_, h_, d_);
		else if (ext == ".png")
			writePng(filepath, pixels_, w_, h_, d_);
		else if (ext == ".ppm")
			writePpm(filepath, pixels_, w_, h_, d_);
		else if (ext == ".tga")
			writeTga(filepath, pixels_, w_, h_, d_);
		else if (ext == ".tiff" || ext == ".tif")
			writeTiff(filepath, pixels_, w_, h_, d_);
		else if (ext == ".webp")
			writeWebp(filepath, pixels_, w_, h_, d_);
		else
		{
			throw std::invalid_argument("Cannot parse filetype");
		}
	}
}