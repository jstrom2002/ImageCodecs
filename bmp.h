#pragma once
#include <string>
#include <vector>

namespace BMP
{
	// NOTE: .bmp files can only contain RGB data. So far, this class does not handle any compressed data or bit depths != 24 bit.
	class Bmp
	{
	public:

		Bmp() {}
		Bmp(std::string filename);
		Bmp(std::vector<std::vector<unsigned char>>& pixelArray, int w, int h);

		const int bitDepth = 24; // For now, pixel depths are hardcoded to 8-bit RGB
		int headerSize;//size of header before pixel array
		int hgt;//gives image width
		int horizontalResolution;
		int imageSize;//gives image size
		int numberOfBytes;//read number of bytes in file
		const int numberOfColorsInPalatte = 0;//colors in palatte, 0 is default (2^n colors). No other values are handled for this class currently.
		int numberOfPixels;//number of total pixel vectors, ie for 24-bit arrays it would be 3 * wdt * hgt
		int pixelsPerRow;//number of pixels in a row, width*3
		int padBytes;//number of bytes padding each row
		int rowSize;//number of bytes necessary to store one row
		int verticalResolution; // resolutions should be either 96 or 72 dpi 
		int wdt;//gives image height 
		const std::string headertype = "BITMAPINFOHEADER";
		std::string colorOrdering;//displays whether the array values are RGB, BGR, grayscale, etc
		std::vector<unsigned char> header;//byte vector of the data for the header to BMP file
		std::vector<std::vector<unsigned char>> pixels;//an array of all the pixels in the BMP file 

		void saveBmp(std::string filename);

	private:
		int calcHorizResolution();
		int calcVertResolution();
		std::string getHeadertype();
		void generateHeader();

		// Helper to calculate values used in load/save
		void updateHeader();
	};
}