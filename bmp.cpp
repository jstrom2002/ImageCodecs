#include "bmp.h"
#include <cmath>
#include <exception>

namespace BMP
{
	Bmp::Bmp(std::vector<std::vector<unsigned char>>& pixelArray, int w, int h)
	{
		colorOrdering = "BGR";
		headerSize = 54;
		wdt = w;
		hgt = h;
		verticalResolution = calcVertResolution();
		horizontalResolution = calcHorizResolution();

		updateHeader();
		generateHeader();

		std::vector<unsigned char> temp;
		for (unsigned int i = 0; i < pixelArray.size(); ++i)
		{
			for (unsigned int j = 0; j < pixelArray[i].size(); ++j)
			{
				temp.push_back(pixelArray[i][j]);
			}
			pixels.push_back(temp);
			temp.clear();
		}
	}

	Bmp::Bmp(std::string filename)
	{
		colorOrdering = "BGR";

		//open BMP file
		FILE* f = fopen(filename.c_str(), "rb");

		//read preliminary file data -- 14 bytes
		unsigned char prelimData[14];
		fread(prelimData, sizeof(unsigned char), 14, f);
		for (int i = 0; i < 14; ++i)
		{
			header.push_back(prelimData[i]);
		}
		unsigned int numberOfBytes = (header[5] << 24) ^ (header[4] << 16) ^ (header[3] << 8) ^ header[2];//read number of bytes in file
		int reservedBytes = (header[9] << 24) ^ (header[8] << 16) ^ (header[7] << 8) ^ header[6];//read reserved data -- unused
		headerSize = (header[13] << 24) ^ (header[12] << 16) ^ (header[11] << 8) ^ header[10];//read starting address
		if (headerSize != 54)
		{
			throw new std::exception("Headers with non-54 byte length are not supported.");
		}
		std::string headertype2 = getHeadertype();
		if (headertype2 != "BITMAPINFOHEADER")
		{
			throw new std::exception("No headers but BITMAPINFOHEADER type are supported.");
		}

		//read and interpret file header data
		unsigned char* headerData = new unsigned char[headerSize - 14];
		fread(headerData, sizeof(unsigned char), headerSize - 14, f); //read rest of the 54-byte header
		for (int i = 0; i < headerSize - 14; ++i) 
		{ 
			header.push_back(headerData[i]); 
		}

		//initialize class variables;
		wdt = (header[21] << 24) ^ (header[20] << 16) ^ (header[19] << 8) ^ header[18];//gives image height 
		hgt = (header[25] << 24) ^ (header[24] << 16) ^ (header[23] << 8) ^ header[22];//gives image width
		horizontalResolution = (header[41] << 24) ^ (header[40] << 16) ^ (header[39] << 8) ^ header[38];
		verticalResolution = (header[45] << 24) ^ (header[44] << 16) ^ (header[43] << 8) ^ header[42];
		int colorDepth = (header[27] << 8) ^ header[28]; //read color depth to determine color array
		if (colorDepth != 24)
		{
			throw new std::exception("Non 8-bit pixel depth is unimplemented");
		}
		int compressionMethod = (header[33] << 24) ^ (header[32] << 16) ^ (header[31] << 8) ^ header[30];//tells what method of compression is used (0 = uncompressed) 
		if (compressionMethod != 0)
		{
			throw new std::exception("Decompression is unimplemented");
		}
		int numberOfColorsInPalatte = (header[49] << 24) ^ (header[48] << 16) ^ (header[47] << 8) ^ header[46];//gives number of colors in color palatte, 0 = 2^n (default)
		if (numberOfColorsInPalatte != 0)
		{
			throw new std::exception("Color palettes != 2^n are unimplemented");
		}
		updateHeader();

		// Get all bytes from image w/ padding interspersed.
		unsigned char c;
		std::vector<unsigned char> data;
		while (fread(&c, sizeof(unsigned char), 1, f)) {
			data.push_back(c);
		}
		fclose(f);

		// Remove padding bits.
		int pixelCount = 0;//counts the number of pixel vectors read
		std::vector<unsigned char> tempVec;
		if (colorDepth == 24) {
			for (int i = 0; i < data.size(); ++i) {
				if (i % rowSize < rowSize - padBytes) {
					if (i > 0 && i % 3 == 0) {
						pixels.push_back(tempVec);
						tempVec.clear();
					}
					tempVec.push_back(data[i]);
				}
			}
		}

	}

	void Bmp::saveBmp(std::string name) {
		FILE* f = fopen(name.c_str(), "wb"); //write file

		//write header
		fwrite(header.data(), sizeof(unsigned char), headerSize, f);

		//write pixel data
		int pixelCount = 0;
		std::vector<unsigned char>row(rowSize,0);
		while (pixelCount < pixels.size()) {
			for (int i = 0; i < rowSize; i += 3) {
				if (padBytes == 0 || (i < pixelsPerRow && pixelCount < pixels.size())) {
					row[i + 0] = pixels[pixelCount][0];
					row[i + 1] = pixels[pixelCount][1];
					row[i + 2] = pixels[pixelCount][2];
					++pixelCount;
				}
				else {
					while (i < rowSize) {
						row[i + 0] = 0;
						++i;
					}
				}
			}
			fwrite(row.data(), sizeof(unsigned char), rowSize, f);
		}

		fclose(f);
	}

	std::string Bmp::getHeadertype() 
	{
		std::string str = "";
		switch (headerSize) 
		{
			case 12 + 14:  str = "BITMAPCOREHEADER"; break;
			case 64 + 14:  str = "OS22XBITMAPHEADER"; break;
			case 16 + 14:  str = "OS22XBITMAPHEADER"; break;
			case 40 + 14:  str = "BITMAPINFOHEADER"; break;
			case 52 + 14:  str = "BITMAPV2INFOHEADER"; break;
			case 56 + 14:  str = "BITMAPV3INFOHEADER"; break;
			case 10 + 14:  str = "BITMAPV4HEADER "; break;
			case 124 + 14: str = "BITMAPV5HEADER"; break;
		}
		return str;
	}

	void Bmp::generateHeader()
	{
		header.clear();
		header = std::vector<unsigned char> 
		{
				0x42, 0x4D, 0x26, 0x4F, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
				0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x8B, 0x01, 0x00, 0x00, 0x5C, 0x01,
				0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x4E,
				0x06, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		header[18] = wdt;
		header[19] = wdt >> 8;
		header[20] = wdt >> 16;
		header[21] = wdt >> 24;

		header[22] = hgt;
		header[23] = hgt >> 8;
		header[24] = hgt >> 16;
		header[25] = hgt >> 24;

		header[38] = horizontalResolution;
		header[39] = horizontalResolution >> 8;
		header[40] = horizontalResolution >> 16;
		header[41] = horizontalResolution >> 24;

		header[42] = verticalResolution;
		header[43] = verticalResolution >> 8;
		header[44] = verticalResolution >> 16;
		header[45] = verticalResolution >> 24;
	}

	int Bmp::calcHorizResolution()
	{
		// TO DO: finish
		return 2834;
	}

	int Bmp::calcVertResolution()
	{
		// TO DO: finish
		return 2834;
	}

	void Bmp::updateHeader() 
	{
		rowSize = std::floor(((bitDepth * wdt) + 31.0) / 32.0) * 4;
		numberOfBytes = headerSize + (rowSize * hgt);
		int temp = verticalResolution;
		verticalResolution = horizontalResolution;
		horizontalResolution = temp;
		imageSize = hgt * rowSize;
		numberOfPixels = 3 * hgt * wdt;
		pixelsPerRow = 3 * wdt;
		padBytes = rowSize - pixelsPerRow;
	}
}