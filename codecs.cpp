#include "codecs.h"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "gif.h"

#define TJE_IMPLEMENTATION
#include "jpeg_enc.h"
#include "jpeg_dec.h"

#define NV_DDS_NO_GL_SUPPORT
#include "nv_dds.h"

#include "png.h"
#include "png_encoder.h"

#include "pnm.h"

extern "C" {
	#include <tiffio.h>
}

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#include "tinyexr.h"

// required libs
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "lzma.lib")
#pragma comment(lib, "miniz.lib")
#pragma comment(lib, "libwebp.lib")
#ifdef NDEBUG
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng16.lib")
#pragma comment(lib, "tiff.lib")
#else
#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "libpng16d.lib")
#pragma comment(lib, "tiffd.lib")
#endif

#include <libpng16/png.h>

#include <webp/encode.h>
#include <webp/decode.h>

namespace ImageCodecs
{
	void Image::read(std::string filepath)
	{
		auto ext = std::filesystem::path(filepath).extension().string();
		for (auto& c : ext)
			c = std::tolower(c);
		if (ext == ".bmp")
			readBmp(filepath, &pixels_, w_, h_, d_,type_);
		else if (ext == ".dds")
			readDds(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".exr")
			readExr(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".gif")
			readGif(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".hdr")
			readHdr(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".jpg" || ext == ".jpeg")
			readJpg(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".png")
			readPng(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".pbm" || ext == ".pfm" || ext == ".pgm" || ext == ".pnm" || ext == ".ppm")
			readPbm(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".tga")
			readTga(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".tif" || ext == ".tiff")
			readTiff(filepath, &pixels_, w_, h_, d_, type_);
		else if (ext == ".webp")
			readWebp(filepath, &pixels_, w_, h_, d_, type_);
		else
		{
			throw std::invalid_argument("Cannot parse filetype");
		}

		if (pixels_ == nullptr)
		{
			throw std::exception("Could not read image data");
		}
	}

	void Image::write(std::string filepath)
	{
		auto ext = std::filesystem::path(filepath).extension().string();
		for (auto& c : ext)
			c = std::tolower(c);
		if (ext == ".bmp")
			writeBmp(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".dds")
			writeDds(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".exr")
			writeExr(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".gif")
			writeGif(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".hdr")
			writeHdr(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".jpg" || ext == ".jpeg")
			writeJpg(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".png")
			writePng(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".pbm" || ext == ".pfm" || ext == ".pgm" || ext == ".pnm" || ext == ".ppm")
			writePbm(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".tga")
			writeTga(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".tiff" || ext == ".tif")
			writeTiff(filepath, pixels_, w_, h_, d_, type_);
		else if (ext == ".webp")
			writeWebp(filepath, pixels_, w_, h_, d_, type_);
		else
		{
			throw std::invalid_argument("Cannot parse filetype");
		}
	}

	void Image::transpose(unsigned char* pixels, const int w, const int h, const int d, const Type& type)
	{
		unsigned int byteSz = byteSize(type);
		unsigned char* tempPix = new unsigned char[totalBytes()];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d * byteSz; k += byteSz)
				{
					unsigned int r = i * w * d * byteSz;
					unsigned int c = j * d * byteSz;
					unsigned int r2 = j * h * d * byteSz;
					unsigned int c2 = i * d * byteSz;

					if (type == Type::FLOAT)
					{
						memcpy(&tempPix[r + c + k], &pixels[r2 + c2 + k], FLOAT_SIZE);
					}
					else
					{
						tempPix[r + c + k] = pixels[r2 + c2 + k];
					}
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < totalBytes(); ++i)
		{
			pixels[i] = tempPix[i];
		}
		delete[] tempPix;
	}


	void Image::flip(unsigned char* pixels, const int w, const int h, const int d, const Type& type)
	{
		auto byteSz = byteSize();
		unsigned char* tempPix = new unsigned char[totalBytes()];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d * byteSz; k += byteSz)
				{
					unsigned int r = i * w * d * byteSz;
					unsigned int r_inv = (h - 1 - i) * w * d * byteSz;
					unsigned int c = j * d * byteSz;

					if (type == Type::FLOAT)
					{
						memcpy(&tempPix[r + c + k], &pixels[r_inv + c + k], FLOAT_SIZE);
					}
					else
					{
						tempPix[r + c + k] = pixels[r_inv + c + k];
					}
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < totalBytes(); ++i)
		{
			pixels[i] = tempPix[i];
		}
		delete[] tempPix;
	}

	void Image::swapBR(unsigned char* pixels, const int w, const int h, const int d, const Type& type)
	{
		unsigned int byteSz = byteSize(type);
		unsigned char* tempPix = new unsigned char[totalBytes()];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d * byteSz; k += byteSz)
				{
					unsigned int r = i * w * d * byteSz;
					unsigned int c = j * d * byteSz;

					if (type == Type::FLOAT)
					{
						if (k == 0)
						{
							memcpy(&tempPix[r + c + 0], &pixels[r + c + 2], FLOAT_SIZE);
						}
						else if (k == 2)
						{
							memcpy(&tempPix[r + c + 2], &pixels[r + c + 0], FLOAT_SIZE);
						}
						else {
							memcpy(&tempPix[r + c + k], &pixels[r + c + k], FLOAT_SIZE);
						}
					}
					else
					{
						if (k == 0)
						{
							tempPix[r + c + 0] = pixels[r + c + 2];
						}
						else if (k == 2)
						{
							tempPix[r + c + 2] = pixels[r + c + 0];
						}
						else {
							tempPix[r + c + k] = pixels[r + c + k];
						}
					}
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < totalBytes(); ++i)
		{
			pixels[i] = tempPix[i];
		}
		delete[] tempPix;
	}

	// Adapted from: https://github.com/marc-q/libbmp/blob/master/CPP/libbmp.cpp
	// NOTE: handles only 3 channel RGB .bmp files with 'BITMAPINFOHEADER' format
	void Image::readBmp(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		const uint32_t BMP_MAGIC = 19778;
		struct {
			unsigned int bfSize = 0;
			unsigned int bfReserved = 0;
			unsigned int bfOffBits = 54;
			unsigned int biSize = 40;
			int biWidth = 0;
			int biHeight = 0;
			unsigned short biPlanes = 1;
			unsigned short biBitCount = 24;
			unsigned int biCompression = 0;
			unsigned int biSizeImage = 0;
			int biXPelsPerMeter = 0;
			int biYPelsPerMeter = 0;
			unsigned int biClrUsed = 0;
			unsigned int biClrImportant = 0;
		} header;

		struct {
			size_t len_row;
			size_t len_pixel = 3;
		} bmpPixBuf;

		std::ifstream f_img(filepath.c_str(), std::ios::binary);
		if (!f_img.is_open())
			throw std::exception("Could not open .bmp file");

		// Check if its an bmp file by comparing the magic nbr
		unsigned short magic;
		f_img.read(reinterpret_cast<char*>(&magic), sizeof(magic));

		if (magic != BMP_MAGIC)
		{
			f_img.close();
			throw std::exception("Could not parse .bmp file");
		}

		// Read the header structure into header
		f_img.read(reinterpret_cast<char*>(&header), sizeof(header));

		// Select the mode (bottom-up or top-down)
		h = std::abs(header.biHeight);
		const int offset = (header.biHeight > 0 ? 0 : h - 1);
		const int padding = (header.biWidth % 4);

		// Allocate the pixel buffer		
		bmpPixBuf.len_row = header.biWidth * bmpPixBuf.len_pixel;
		*pixels = new unsigned char[h * bmpPixBuf.len_row];		

		for (int y = h - 1; y >= 0; y--)
		{
			// Read a whole row of pixels from the file
			f_img.read(reinterpret_cast<char*> (&(*pixels)[(int)std::abs(y - offset) * bmpPixBuf.len_row]), bmpPixBuf.len_row);

			// Skip the padding
			f_img.seekg(padding, std::ios::cur);
		}

		h = header.biHeight;
		w = header.biWidth;
		d = 3;

		f_img.close();
	}

	// Adapted from: https://github.com/marc-q/libbmp/blob/master/CPP/libbmp.cpp
	// NOTE: handles only 3 channel RGB .bmp files with 'BITMAPINFOHEADER' format
	void Image::writeBmp(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		const uint32_t BMP_MAGIC = 19778;
		struct {
			unsigned int bfSize = 0;
			unsigned int bfReserved = 0;
			unsigned int bfOffBits = 54;
			unsigned int biSize = 40;
			int biWidth = 0;
			int biHeight = 0;
			unsigned short biPlanes = 1;
			unsigned short biBitCount = 24;
			unsigned int biCompression = 0;
			unsigned int biSizeImage = 0;
			int biXPelsPerMeter = 0;
			int biYPelsPerMeter = 0;
			unsigned int biClrUsed = 0;
			unsigned int biClrImportant = 0;
		} header;

		header.bfSize = (3 * w + (w % 4)) * h;
		header.biWidth = w;
		header.biHeight = h;

		// Open the image file in binary mode
		std::ofstream f_img(filepath.c_str(), std::ios::binary);

		if (!f_img.is_open())
			throw std::exception("Could not open .bmp file to write");

		// Since an adress must be passed to fwrite, create a variable!
		const unsigned short magic = BMP_MAGIC;

		f_img.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
		f_img.write(reinterpret_cast<const char*>(&header), sizeof(header));

		// Select the mode (bottom-up or top-down)
		const int offset = (header.biHeight > 0 ? 0 : h - 1);
		const int padding = header.biWidth % 4;

		for (int y = h - 1; y >= 0; y--)
		{
			// Write a whole row of pixels into the file
			uint32_t len_row = w * d;
			f_img.write(reinterpret_cast<char*> (&pixels[(int)std::abs(y - offset) * len_row]), len_row);

			// Write the padding
			f_img.write("\0\0\0", padding);
		}

		f_img.close();
	}

	void Image::readDds(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		nv_dds::CDDSImage image;
		nv_dds::CSurface surf;
		int totalBytes_ = 0;
		try
		{
			image.load(filepath, true);
		}
		catch (std::exception e1)
		{
			throw std::exception(e1.what());
		}

		w = image.get_width();
		h = image.get_height();
		d = image.get_components();
		totalBytes_ = image.get_size();

		if (image.get_num_mipmaps() > 1) {
			surf = image.get_mipmap(0);
			w = surf.get_width();
			h = surf.get_height();
			d = surf.get_depth();
			totalBytes_ = surf.get_size();
		}

		switch (image.get_type()) {
		case nv_dds::TextureType::TextureFlat:
			break;
		case nv_dds::TextureType::TextureCubemap:
			throw std::exception("Cannot handle .dds cubemap textures");
		case nv_dds::TextureType::Texture3D:
			throw std::exception("Cannot handle .dds 3D textures");
		}

		if (totalBytes_ / (w * h * d) != 1)
		{
			if (totalBytes_ / (w * h * d) == 4)
			{
				type = Type::FLOAT;
			}
			else if (totalBytes_ / (w * h * d) == 2)
			{
				type = Type::USHORT;
			}
		}

		*pixels = new unsigned char[totalBytes()];
		unsigned int byte_counter = 0;		
		if (image.get_num_mipmaps() > 1) 
		{
			memcpy(*pixels, surf, totalBytes());
		}
		else
		{			
			memcpy(*pixels, image, totalBytes());
		}

		flip(*pixels, w, h, d, type);

		surf.clear();
		image.clear();		
	}
	void Image::writeDds(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		nv_dds::CTexture img;
		img.create(w,h,1,totalBytes(),pixels);	
		nv_dds::CDDSImage ddsimage;		
		unsigned int fmt = 0;
		switch (d) {
		case 1:
			fmt = 6403;//GL_RED
			break;
		case 3:
			fmt = GL_RGB;
			break;
		case 4:
			fmt = GL_RGBA;
			break;
		}
		ddsimage.create_textureFlat(fmt, d, img);		
		ddsimage.save(filepath);
		img.clear();
		ddsimage.clear();
	}

	void Image::readExr(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		std::vector<unsigned char> loadedData;
		std::ifstream ifile(filepath.c_str(), std::ios::in | std::ios::binary);
		while (ifile.good())
		{
			loadedData.push_back(ifile.get());
		}
		ifile.close();
		float* floatPixels;
		const char* err = "\0";
		auto ret = LoadEXRFromMemory(&floatPixels, &w, &h, loadedData.data(), loadedData.size(), &err);
		loadedData.clear();
		if (!ret)
		{
			type = Type::FLOAT;
			d = 4; // assume RGBA float data
			unsigned int sz = totalBytes();
			*pixels = new unsigned char[sz];
			memcpy(*pixels, floatPixels, sz);
		}
		else
		{
			std::cerr << err << std::endl;
			FreeEXRErrorMessage(err);
			throw std::exception("Could not load .exr");
		}
		type = Type::FLOAT;
		delete[] floatPixels;
	}

	void Image::writeExr(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		const char* err = "\0";
		float* float_pixels = new float[w * h * d];
		memcpy(float_pixels, pixels, w * h * d * FLOAT_SIZE);
		auto ret = SaveEXR(float_pixels, w, h, d, false, filepath.c_str(), &err);
		if (ret)
		{
			std::cerr << err << std::endl;
		}
	}

	void Image::readGif(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		gif::gd_GIF* gif;
		gif = gif::gd_open_gif(filepath.c_str());
		if (!gif) {
			throw std::exception("Could not open gif file");
		}

		w = gif->width;
		h = gif->height;
		d = 3; // Assumes 3-channel RGB encoding of data, which is standard for .gif images.

		uint8_t* frame = new uint8_t[totalBytes()];

		switch (gif->depth)
		{
		case 16:
			type = Type::USHORT;
			break;
		case 32:
			type = Type::FLOAT;
			break;
		default:
			type = Type::UBYTE;
			break;
		}

		*pixels = new unsigned char[totalBytes()];
		unsigned int counter = 0;

		// Get only the first frame of the .gif file, although this could be called repeatedly to get all of them.
		if (gd_get_frame(gif) == -1)
			throw std::exception("Could not load .gif data");
		gd_render_frame(gif, frame);
		memcpy(*pixels, frame, totalBytes());		
	}

	void Image::writeGif(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		gif::CGIF* pGIF;			
		gif::CGIF_Config gConfig;    
		memset(&gConfig, 0, sizeof(gif::CGIF_Config));
		gif::CGIF_FrameConfig fConfig;
		memset(&fConfig, 0, sizeof(gif::CGIF_FrameConfig));

		// Assumes 3-channel RGB encoding of data.
		uint8_t colTable[256*3]; // generate a 3 channel 8-bit global color palette for use.
		unsigned int counter = 0;
		for (unsigned int i = 0; i < 3; ++i)
			for (unsigned int j = 0; j < 256; ++j)
			{
				colTable[counter] = j;
				counter++;
			}
		gConfig.width = w;
		gConfig.height = h;
		gConfig.numGlobalPaletteEntries = 256;
		gConfig.pGlobalPalette = colTable;
		gConfig.path = filepath.c_str();

		// add frame to GIF
		pGIF = cgif_newgif(&gConfig);	
		uint8_t* px = new uint8_t[totalBytes()];
		
		// Organize pixel data so that each channel is written one-at-a-time.
		counter = 0;
		for(unsigned int k = 0; k < d; ++k)
			for (unsigned int i = 0; i < h; ++i)
				for (unsigned int j = 0; j < w; ++j)
					for (unsigned int l = 0; l < byteSize(); ++l)
					{
						px[counter] = pixels[i * (w*d*byteSize()) + j * (d * byteSize()) + k * byteSize() + l];
						counter++;
					}

		fConfig.pImageData = px;
		fConfig.delay = 0;
		auto err = cgif_addframe(pGIF, &fConfig);
		if (err) // add a new frame to the GIF
		{
			delete[] px;			
			throw std::exception(("Could not assign frame data. Code: " + std::to_string(err)).c_str());
		}		

		// close GIF and free allocated space
		cgif_close(pGIF);
		delete[] px;
	}

	typedef unsigned char RGBE[4];
#define R			0
#define G			1
#define B			2
#define E			3

#define  MINELEN	8				// minimum scanline length for encoding
#define  MAXELEN	0x7fff			// maximum scanline length for encoding
	int invConvertComponent(int expo, float val)
	{
		float d = (float)pow(0.5f, float(expo));
		return unsigned char(val * 256.0f * d);
	}

	float convertComponent(int expo, int val)
	{
		float v = val / 256.0f;
		float d = (float)pow(2, expo);
		return v * d;
	}

	void workOnRGBE(RGBE* scan, int len, float* cols)
	{
		while (len-- > 0) {
			int expo = scan[0][E] - 128;
			cols[0] = convertComponent(expo, scan[0][R]);
			cols[1] = convertComponent(expo, scan[0][G]);
			cols[2] = convertComponent(expo, scan[0][B]);
			cols[3] = float(scan[0][E]);
			cols += 4;
			scan++;
		}
	}

	bool oldDecrunchHDR(RGBE* scanline, int len, FILE* file)
	{
		int i;
		int rshift = 0;

		while (len > 0) {
			scanline[0][R] = fgetc(file);
			scanline[0][G] = fgetc(file);
			scanline[0][B] = fgetc(file);
			scanline[0][E] = fgetc(file);
			if (feof(file))
				return false;

			if (scanline[0][R] == 1 &&
				scanline[0][G] == 1 &&
				scanline[0][B] == 1) {
				for (i = scanline[0][E] << rshift; i > 0; i--) {
					memcpy(&scanline[0][0], &scanline[-1][0], 4);
					scanline++;
					len--;
				}
				rshift += 8;
			}
			else {
				scanline++;
				len--;
				rshift = 0;
			}
		}
		return true;
	}

	bool decrunchHDR(RGBE* scanline, int len, FILE* file)
	{
		int  i, j;

		if (len < MINELEN || len > MAXELEN)
			return oldDecrunchHDR(scanline, len, file);

		i = fgetc(file);
		if (i != 2) {
			fseek(file, -1, SEEK_CUR);
			return oldDecrunchHDR(scanline, len, file);
		}

		scanline[0][G] = fgetc(file);
		scanline[0][B] = fgetc(file);
		i = fgetc(file);

		if (scanline[0][G] != 2 || scanline[0][B] & 128) {
			scanline[0][R] = 2;
			scanline[0][E] = i;
			return oldDecrunchHDR(scanline + 1, len - 1, file);
		}

		// read each component
		for (i = 0; i < 4; i++) {
			for (j = 0; j < len; ) {
				unsigned char code = fgetc(file);
				if (code > 128) { // run
					code &= 127;
					unsigned char val = fgetc(file);
					while (code--)
						scanline[j++][i] = val;
				}
				else {	// non-run
					while (code--)
						scanline[j++][i] = fgetc(file);
				}
			}
		}

		return feof(file) ? false : true;
	}


	void Image::readHdr(std::string filepath, unsigned char** pixels, int& w_, int& h_, int& d_, Type& type)
	{
		int i;
		char str[200];
		FILE* file;

		file = fopen(filepath.c_str(), "rb");
		if (!file)
			throw std::exception("Cannot open file");

		fread(str, 10, 1, file);
		if (memcmp(str, "#?RADIANCE", 10)) {
			fclose(file);
			throw std::exception("Invalid file format");
		}

		fseek(file, 1, SEEK_CUR);

		char cmd[200];
		i = 0;
		char c = 0, oldc;
		while (true) {
			oldc = c;
			c = fgetc(file);
			if (c == 0xa && oldc == 0xa)
				break;
			cmd[i++] = c;
		}

		char reso[200];
		i = 0;
		while (true) {
			c = fgetc(file);
			reso[i++] = c;
			if (c == 0xa)
				break;
		}

		int w, h;
		if (!sscanf(reso, "-Y %ld +X %ld", &h, &w)) {
			fclose(file);
			throw std::exception("Invalid file format");
		}

		w_ = w;
		h_ = h;
		d_ = 4; // All .hdr files are 4-channel RGBE data w/ alpha component == exposure
		float* floatPixels = new float[w * h * d_];

		RGBE* scanline = new RGBE[w];
		if (!scanline) {
			fclose(file);
			throw std::exception("Invalid file format");
		}

		// convert image and copy over float data.
		type = Type::FLOAT;
		*pixels = new unsigned char[totalBytes()];
		unsigned int lineCount = 0;
		for (int y = h - 1; y >= 0; y--) {
			if (decrunchHDR(scanline, w, file) == false)
				break;
			workOnRGBE(scanline, w, floatPixels);
			memcpy(*pixels + lineCount, floatPixels, w * d_ * FLOAT_SIZE);
			floatPixels += w * d_;
			lineCount += w * d_ * FLOAT_SIZE;
		}
		
		// Cleanup.
		delete[] scanline;
		fclose(file);
	}

	void Image::writeHdr(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		if (d != 4)
		{
			throw std::exception("HDR data must contain a 4th channel of exposure values");
		}

		std::ofstream ofile(filepath.c_str(), std::ios::out | std::ios::binary);
		if (!ofile.is_open())
			throw std::exception("Cannot open output file.");

		// Write hdr header.
		ofile << "#?RADIANCE" << char(0x0A) << "SOFTWARE=GEGL" << char(0x0A) << "FORMAT=32-bit_rle_rgbe" << char(0x0A) << char(0x0A) << "-Y " << std::to_string(h) << " +X " << std::to_string(w) << char(0x0A);

		unsigned int counter = 0;
		float px;
		float expo2;
		for (unsigned int i = 0; i < w * h; ++i) // assumes size of float == 4 bytes
		{
			memcpy(&expo2, &pixels[counter+(3 * FLOAT_SIZE)], FLOAT_SIZE);
			int expo = int(expo2) - 128;
			for (unsigned int j = 0; j < d; ++j)
			{
				if (j == 3) // Save exposure float
				{
					ofile << unsigned char(expo+128);
				}
				else // For RGB data, convert back to linear value and write to file.
				{
					px = 0.0f;
					memcpy(&px, &pixels[counter], FLOAT_SIZE);
					ofile << (unsigned char)invConvertComponent(px,expo);
					
				}
				counter+=4; // assumes size of float == 4 bytess
			}
		}

		ofile.flush();
		ofile.close();
	}

	void Image::readJpg(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		// Open file.
		FILE* f = fopen(filepath.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		int size = (int)ftell(f);
		auto temparr = new unsigned char[size];
		fseek(f, 0, SEEK_SET);
		size = (int)fread(temparr, 1, size, f);
		fclose(f);

		// Init jpeg lib and decode bytes.
		njInit();
		if (njDecode(temparr, size)) {
			delete[] temparr;
			throw std::exception("Error decoding the input file.\n");
		}

		// Copy results to local vars.
		d = 3;
		w = njGetWidth();
		h = njGetHeight();
		*pixels = new unsigned char[totalBytes()];
		memcpy(*pixels, njGetImage(), totalBytes());

		// Cleanup.
		delete[] temparr;
		njDone();
	}

	void Image::writeJpg(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		tje_encode_to_file(filepath.c_str(), w, h, d, pixels);
	}

	typedef struct {
		const png_byte* data;
		const png_size_t size;
	} DataHandle;

	typedef struct {
		const DataHandle data;
		png_size_t offset;
	} ReadDataHandle;

#define PNG_SIG_BYTES (8) /* bytes in the PNG file signature. */
#define PNG_RGBA_PIXEL_LIMIT (0x1000000)

	size_t Read(unsigned char* dest, unsigned char* Data, const size_t byteCount) {
		std::copy(Data + 0, Data + byteCount, dest);
		return byteCount;
	}

	void ReadDataFromInputStream(png_structp png_ptr, png_byte* raw_data, png_size_t read_length) {
		ReadDataHandle* handle = (ReadDataHandle*)png_get_io_ptr(png_ptr);
		const png_byte* png_src = handle->data.data + handle->offset;
		memcpy(raw_data, png_src, read_length);
		handle->offset += read_length;
	}

	static int png_rgba_pixel_limit(png_uint_32 w, png_uint_32 h) {
		double da;
		/* assert(w != 0 && h != 0); */
		if (w > PNG_RGBA_PIXEL_LIMIT || h > PNG_RGBA_PIXEL_LIMIT)
			return (1); /* since both (w) and (h) are non-zero. */
		/* since an IEEE-754 double has a 53 bit mantissa, it can
		* represent the maximum area: (w * h == 2^48) exactly. */
		da = ((double)w) * ((double)h);
		if (da > ((double)PNG_RGBA_PIXEL_LIMIT))
			return (1);
		return (0); /* the PNG image is within the pixel limit. */
	}

	unsigned int accessArray3D(unsigned int r, unsigned int c, unsigned int d, unsigned int rows, unsigned int cols, unsigned int depth) {
		unsigned int idx = ((r * cols + c) * depth) + d;
		if (idx >= cols * rows * depth) {
			throw std::exception("ERROR! Indexing outside of array");
		}
		return idx;
	}

	//unsigned char* LoadPng(unsigned char* Data, int& width, int& height, int& depth, bool flip)	
	void Image::readPng(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		unsigned int sz = std::filesystem::file_size(filepath);
		unsigned char* Data = new unsigned char[sz];
		std::ifstream ifile(filepath, std::ios::in | std::ios::binary);
		ifile.read(reinterpret_cast<char*>(Data), sz);		
		ifile.close();

		png_byte magic[PNG_SIG_BYTES]; /* (signature byte buffer) */
		png_structp png_ctx;
		png_infop info_ctx;
		png_uint_32 img_width, img_height, row;
		png_byte img_depth, img_color_type;

		/* 'volatile' qualifier forces reload in setjmp cleanup: */
		unsigned char* volatile img_data = NULL;
		png_bytep* volatile row_data = NULL;

		;//*buf = NULL;

		 /* it is assumed that 'longjmp' can be invoked within this
		 * code to efficiently unwind resources for *all* errors. */
		 /* PNG structures and resource unwinding: */
		if ((png_ctx = png_create_read_struct(
			PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
			return; /* ENOMEM (?) */
		if ((info_ctx = png_create_info_struct(png_ctx)) == NULL)
		{
			png_destroy_read_struct(&png_ctx, NULL, NULL);
			return; /* ENOMEM (?) */
		}
		if (setjmp(png_jmpbuf(png_ctx)) != 0)
		{
			png_destroy_read_struct(&png_ctx, &info_ctx, NULL);
			free(img_data); free(row_data);
			return; /* libpng feedback (?) */
		}

		/* check PNG file signature: */
		Read(magic, Data, PNG_SIG_BYTES);

		if (png_sig_cmp(magic, 0, PNG_SIG_BYTES))
			png_error(png_ctx, "invalid PNG file");

		/* set the input file stream and get the PNG image info: */
		ReadDataHandle a = ReadDataHandle{ { Data, 898 }, 0 };
		png_set_read_fn(png_ctx, &a, ReadDataFromInputStream);

		//////////////// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		png_read_info(png_ctx, info_ctx);
		img_width = png_get_image_width(png_ctx, info_ctx);
		img_height = png_get_image_height(png_ctx, info_ctx);

		if (img_width == 0 || img_height == 0)
			png_error(png_ctx, "zero area PNG image");
		if (png_rgba_pixel_limit(img_width, img_height))
			png_error(png_ctx, "PNG image exceeds pixel limits");

		img_depth = png_get_bit_depth(png_ctx, info_ctx);
		img_color_type = png_get_color_type(png_ctx, info_ctx);

		/* ignored image interlacing, compression and filtering. */
		/* force 8-bit color channels: */
		if (img_depth == 16)
			png_set_strip_16(png_ctx);
		else if (img_depth < 8)
			png_set_packing(png_ctx);
		/* force formats to RGB: */
		if (img_color_type != PNG_COLOR_TYPE_RGBA)
			png_set_expand(png_ctx);
		if (img_color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png_ctx);
		if (img_color_type == PNG_COLOR_TYPE_GRAY)
			png_set_gray_to_rgb(png_ctx);
		if (img_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png_ctx);
		/* add full opacity alpha channel if required: */
		if (img_color_type != PNG_COLOR_TYPE_RGBA)
			png_set_filler(png_ctx, 0xff, PNG_FILLER_AFTER);

		/* apply the output transforms before reading image data: */
		png_read_update_info(png_ctx, info_ctx);

		/* allocate RGBA image data: */
		img_data = (png_byte*)
			malloc((size_t)(img_width * img_height * (4)));

		if (img_data == NULL)
			png_error(png_ctx, "error allocating image buffer");

		/* allocate row pointers: */
		row_data = (png_bytep*)
			malloc((size_t)(img_height * sizeof(png_bytep)));
		if (row_data == NULL)
			png_error(png_ctx, "error allocating row pointers");

		/* set the row pointers and read the RGBA image data: */
		for (row = 0; row < img_height; row++)
			row_data[row] = img_data +
			(img_height - (row + 1)) * (img_width * (4));
		png_read_image(png_ctx, row_data);

		/* libpng and dynamic resource unwinding: */
		png_read_end(png_ctx, NULL);
		png_destroy_read_struct(&png_ctx, &info_ctx, NULL);
		free(row_data);

		w = img_width;
		h = img_height;
		d = 4; // Forced channel number to RGBA == 4.

		delete[] Data;
		*pixels = new unsigned char[totalBytes()];
		memcpy(*pixels, img_data, totalBytes());

		flip(*pixels, w, h, d, type);
	}

	void Image::writePng(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		png_encoder::saveToFile(filepath,pixels,w,h,d);
	}

	int getBit(int whichBit)
	{
		if (whichBit > 0 && whichBit <= 8)
			return (1 << (whichBit - 1));
		else
			return 0;
	}
    void Image::readPbm(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {
		std::vector<std::uint8_t> data;
		std::ifstream ifile(filepath, std::ios::in|std::ios::binary);
		PNM::Info info;

		ifile >> PNM::load(data,info);
		w = info.width();
		h = info.height();
		d = info.channel();
		bool isPfm = filepath.find(".pfm") != std::string::npos;
		if (isPfm) // .pfm files contain float data
		{
			type = Type::FLOAT;
		}
		*pixels = new unsigned char[totalBytes()];
		if (filepath.find(".pbm") != std::string::npos) // .pbm P4 files are binary black/white, so extract 8 bits of pixel data per byte
		{
			if (info.type() == PNM::P4) // binary pixel data in 1's and 0's
			{
				int lastRowBits = w % 8;
				unsigned int counter = 0;
				bool endOfRow = false;
				bool rowJustEnded = false;
				for (unsigned int i = 0; i < data.size(); ++i)
				{
					for (unsigned int j = 0; j < 8; j++)
					{
						if (!rowJustEnded && counter > 0 && counter % w == 0) // skip padding bits in byte at end of row.
						{
							endOfRow = true;
						}
						else
						{
							rowJustEnded = false;
						}

						if (endOfRow && j >= lastRowBits)
						{
							endOfRow = false;
							rowJustEnded = true;
							break;
						}

						unsigned char px = (data[i] >> (7 - j)) & 0x01;
						(*pixels)[counter] = px > 0 ? 0 : 255;
						counter++;
					}

					endOfRow = false;
				}
			}
			else if (info.type() == PNM::P1) // binary ASCII
			{
				int sdf = 0;//DELETE THIS
			}
			else
			{

			}
		}
		else
		{
			memcpy(*pixels, data.data(), totalBytes());
		}

		if (isPfm)
		{
			flip(*pixels, w, h, d, type);
		}
    }

    void Image::writePbm(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
  //      std::ofstream outfile(filepath, std::ofstream::binary);
  //      if (outfile.fail())
  //      {
  //          throw std::exception("Failed to write ");
  //      }

		//if (filepath.find(".pbm") != std::string::npos)
		//{
		//	outfile << "P4" << "\n" << w << " " << h << "\n";

		//	int lastRowBits = w % 8;
		//	unsigned int counter = 0;
		//	bool endOfRow = false;
		//	bool rowJustEnded = false;
		//	while (counter < (w * h * d) + (h * lastRowBits))
		//	{
		//		unsigned char byteToWrite = 0;
		//		for (unsigned int j = 0; j < 8; j++)
		//		{
		//			if (!rowJustEnded && counter > 0 && counter % w == 0) // skip padding bits in byte at end of row.
		//			{
		//				endOfRow = true;
		//			}
		//			else
		//			{
		//				rowJustEnded = false;
		//			}

		//			if (endOfRow && j >= lastRowBits)
		//			{
		//				endOfRow = false;
		//				rowJustEnded = true;
		//				break;^
		//			}
		//			
		//			unsigned char px = (pixels[counter] > 0 ? 0x00 : 0x01);
		//			byteToWrite |= (px << (7-j));
		//			counter++;
		//		}				
		//		outfile << byteToWrite;
		//		endOfRow = false;
		//	}
		//}
		//else if (filepath.find(".pfm") != std::string::npos)
		//{
		//	if (type != Type::FLOAT)
		//	{
		//		throw std::exception("Cannot write non-float data to .pfm");
		//	}

		//	outfile << (d == 3 ? "PF" : "Pf") << (char)0x0A << w << " " << h << (char)0x0A << "-1.0" << (char)0x0A;
		//	outfile.write(reinterpret_cast<char*>(pixels), totalBytes()); // write binary
		//}
		//else if (filepath.find(".pgm") != std::string::npos || filepath.find(".ppm") != std::string::npos || filepath.find(".pnm") != std::string::npos)
		//{
		//	bool isPgm = filepath.find(".pgm") != std::string::npos;
		//	outfile << (isPgm ? "P5" : "P6") << "\n" << w << " " << h << "\n" << 255 << "\n";
		//	outfile.write(reinterpret_cast<char*>(pixels), totalBytes()); // write binary
		//}
		//else
		//{
		//	throw std::exception("Could not recognize netpbm format");
		//}
    }

	template <typename Type>
	void RGBPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size) {
		const int PixelSize = 3;
		Type Index;
		uint8_t Red, Green, Blue;
		uint8_t* ColorMapPtr;
		for (size_t i = 0; i < Size; i++) {
			Index = InBuffer[i];
			ColorMapPtr = &ColorMap[Index * PixelSize];
			Blue = *ColorMapPtr++;
			Green = *ColorMapPtr++;
			Red = *ColorMapPtr++;
			*OutBuffer++ = Red;
			*OutBuffer++ = Green;
			*OutBuffer++ = Blue;
		}
	}

	template <typename Type>
	void RGBAPaletted(Type* InBuffer, uint8_t* ColorMap, uint8_t* OutBuffer, size_t Size) {
		const int PixelSize = 4;
		Type Index;
		uint8_t Red, Green, Blue, Alpha;
		uint8_t* ColorMapPtr;
		for (size_t i = 0; i < Size; i++) {
			Index = InBuffer[i];
			ColorMapPtr = &ColorMap[Index * PixelSize];
			Blue = *ColorMapPtr++;
			Green = *ColorMapPtr++;
			Red = *ColorMapPtr++;
			Alpha = *ColorMapPtr++;
			*OutBuffer++ = Red;
			*OutBuffer++ = Green;
			*OutBuffer++ = Blue;
			*OutBuffer++ = Alpha;
		}
	}

	void MonochromeCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size) {
		uint8_t Header;
		uint8_t Red;
		size_t i, j, PixelCount;
		for (i = 0; i < Size; ) {
			Header = *InBuffer++;
			PixelCount = (Header & 0x7F) + 1;
			if (Header & 0x80) {
				Red = *InBuffer++;
				for (j = 0; j < PixelCount; j++)
					*OutBuffer++ = Red;
				i += PixelCount;
			}
			else {
				for (j = 0; j < PixelCount; j++) {
					Red = *InBuffer++;
					*OutBuffer++ = Red;
				}
				i += PixelCount;
			}
		}
	}

	void RGBCompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size) {
		uint8_t Header;
		uint8_t Red, Green, Blue;
		size_t i, j, PixelCount;
		for (i = 0; i < Size; ) {
			Header = *InBuffer++;
			PixelCount = (Header & 0x7F) + 1;

			if (Header & 0x80) {
				Blue = *InBuffer++;
				Green = *InBuffer++;
				Red = *InBuffer++;
				for (j = 0; j < PixelCount; j++) {
					*OutBuffer++ = Red;
					*OutBuffer++ = Green;
					*OutBuffer++ = Blue;
				}
				i += PixelCount;
			}
			else {
				for (j = 0; j < PixelCount; j++) {
					Blue = *InBuffer++;
					Green = *InBuffer++;
					Red = *InBuffer++;
					*OutBuffer++ = Red;
					*OutBuffer++ = Green;
					*OutBuffer++ = Blue;
				}
				i += PixelCount;
			}
		}
	}

	void RGBACompressed(uint8_t* InBuffer, uint8_t* OutBuffer, size_t Size) {
		uint8_t Header;
		uint8_t Red, Green, Blue, Alpha;
		size_t i, j, PixelCount;

		for (i = 0; i < Size; ) {
			Header = *InBuffer++;
			PixelCount = (Header & 0x7F) + 1;

			if (Header & 0x80) {
				Blue = *InBuffer++;
				Green = *InBuffer++;
				Red = *InBuffer++;
				Alpha = *InBuffer++;

				for (j = 0; j < PixelCount; j++) {
					*OutBuffer++ = Red;
					*OutBuffer++ = Green;
					*OutBuffer++ = Blue;
					*OutBuffer++ = Alpha;
				}
				i += PixelCount;
			}
			else {
				for (j = 0; j < PixelCount; j++) {
					Blue = *InBuffer++;
					Green = *InBuffer++;
					Red = *InBuffer++;
					Alpha = *InBuffer++;

					*OutBuffer++ = Red;
					*OutBuffer++ = Green;
					*OutBuffer++ = Blue;
					*OutBuffer++ = Alpha;
				}
				i += PixelCount;
			}
		}
	}

	// Modified from: https://github.com/ColumbusUtrigas/TGA
	void Image::readTga(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		std::ifstream File(filepath, std::ios::binary);
		if (!File.is_open()) return;
		struct Header {
			uint8_t IDLength;
			uint8_t ColorMapType;
			uint8_t ImageType;
			uint16_t ColorMapOrigin;
			uint16_t ColorMapLength;
			uint8_t  ColorMapEntrySize;
			uint16_t XOrigin;
			uint16_t YOrigin;
			uint16_t Width;
			uint16_t Height;
			uint8_t  Bits;
			uint8_t  ImageDescriptor;
		} Head;
		size_t FileSize = std::filesystem::file_size(filepath);
		File.read((char*)&Head.IDLength, sizeof(Head.IDLength));
		File.read((char*)&Head.ColorMapType, sizeof(Head.ColorMapType));
		File.read((char*)&Head.ImageType, sizeof(Head.ImageType));
		File.read((char*)&Head.ColorMapOrigin, sizeof(Head.ColorMapOrigin));
		File.read((char*)&Head.ColorMapLength, sizeof(Head.ColorMapLength));
		File.read((char*)&Head.ColorMapEntrySize, sizeof(Head.ColorMapEntrySize));
		File.read((char*)&Head.XOrigin, sizeof(Head.XOrigin));
		File.read((char*)&Head.YOrigin, sizeof(Head.YOrigin));
		File.read((char*)&Head.Width, sizeof(Head.Width));
		File.read((char*)&Head.Height, sizeof(Head.Height));
		File.read((char*)&Head.Bits, sizeof(Head.Bits));
		File.read((char*)&Head.ImageDescriptor, sizeof(Head.ImageDescriptor));
		uint8_t* Descriptor = new uint8_t[Head.ImageDescriptor];
		File.read((char*)Descriptor, Head.ImageDescriptor);
		size_t ColorMapElementSize = Head.ColorMapEntrySize / 8;
		size_t ColorMapSize = Head.ColorMapLength * ColorMapElementSize;
		uint8_t* ColorMap = new uint8_t[ColorMapSize];
		if (Head.ColorMapType == 1)
			File.read((char*)ColorMap, ColorMapSize);
		size_t PixelSize = Head.ColorMapLength == 0 ? (Head.Bits / 8) : ColorMapElementSize;
		size_t DataSize = FileSize - sizeof(Header) - (Head.ColorMapType == 1 ? ColorMapSize : 0);
		size_t ImageSize = Head.Width * Head.Height * PixelSize;
		uint8_t* Buffer = new uint8_t[DataSize];
		File.read((char*)Buffer, DataSize);
		*pixels = new unsigned char[ImageSize];
		memset(*pixels, 0, ImageSize);
		switch (Head.ImageType) {
			case 0: break; // No Image
			case 1: {// Uncompressed paletted		
				if (Head.Bits == 8) {
					switch (PixelSize) {
					case 3: RGBPaletted((uint8_t*)Buffer, ColorMap, *pixels, Head.Width * Head.Height); break;
					case 4: RGBAPaletted((uint8_t*)Buffer, ColorMap, *pixels, Head.Width * Head.Height); break;
					}
				}
				else if (Head.Bits == 16) {
					switch (PixelSize) {
					case 3: RGBPaletted((uint16_t*)Buffer, ColorMap, *pixels, Head.Width * Head.Height); break;
					case 4: RGBAPaletted((uint16_t*)Buffer, ColorMap, *pixels, Head.Width * Head.Height); break;
					}
				}
				break;
			}
			case 2: {// Uncompressed TrueColor		
				if (Head.Bits = 24 || Head.Bits == 32) {
					std::copy(&Buffer[0], &Buffer[ImageSize], *pixels);

					// Swap R<->B channels.
					for (size_t i = 0; i < ImageSize; i += PixelSize)
					{
						std::swap((*pixels)[i], (*pixels)[i + 2]);						
					}
				}
				break;
			}
			case 3: {// Uncompressed Monochrome		
				if (Head.Bits == 8)
					std::copy(&Buffer[0], &Buffer[ImageSize], &(*pixels)[0]);
				break;
			}
			case 9: break; // Compressed paletted TODO
			case 10: {// Compressed TrueColor		
				switch (Head.Bits)
				{
				case 24: RGBCompressed(Buffer, *pixels, Head.Width * Head.Height); break;
				case 32: RGBACompressed(Buffer, *pixels, Head.Width * Head.Height); break;
				}
				break;
			}
			case 11: {// Compressed Monocrhome		
				if (Head.Bits == 8)
					MonochromeCompressed(Buffer, *pixels, Head.Width * Head.Height);
				break;
			}
		}

		if (Head.ImageType != 0) {
			d = PixelSize;
			w = Head.Width;
			h = Head.Height;
			flip(*pixels, w, h, d, type); // for some reason, this code loads the .tga data upside down
		}
		delete[] ColorMap;
		delete[] Descriptor;
	}

	// NOTE: only writes uncompressed .tga for now.
    void Image::writeTga(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
        FILE* fp = NULL;
        fp = fopen(filepath.c_str(), "wb");
        if (fp == NULL)
            return;

        unsigned char header[18] =
        {
            0,0,2,0,0,0,0,0,0,0,0,0,
            (unsigned char)(w % 256),
            (unsigned char)(w / 256),
            (unsigned char)(h % 256),
            (unsigned char)(h / 256),
            (unsigned char)(d * 8),
            0x20
        };
        fwrite(&header, 18, 1, fp);

        for (int i = 0; i < w * h; i++)
        {
            for (int b = 0; b < d; b++)
            {
                fputc(pixels[(i * d) + (b % d)], fp);
            }
        }
        fclose(fp);
    }

	void Image::readTiff(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {
		TIFF* tif = TIFFOpen(filepath.c_str(), "r");
		uint32_t* buf;
		tsize_t scanlineSz;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &d);
		int bitDepth = 0;
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitDepth);
		if (bitDepth > 8)
		{
			switch (bitDepth)
			{
			case 16:
				type = Type::USHORT;
			case 32:
				type = Type::FLOAT;
			}
		}
		*pixels = new unsigned char[totalBytes()];

		scanlineSz = TIFFScanlineSize(tif);
		buf = new uint32_t[w*h];//(tdata_t*)_TIFFmalloc(scanlineSz);
		if (!TIFFReadRGBAImage(tif, (uint32_t)w, (uint32_t)h, (uint32_t*)buf, 1))
		{
			std::cerr << "Error reading .tiff file" << std::endl;
			throw std::exception("Error reading .tff file");
		}
		unsigned int counter = 0;

		for (unsigned int i = 0; i < w * h; ++i)
		{
			uint32_t currPix = (uint32_t)buf[i];
			for (unsigned int j = 0; j < d; ++j)
			{
				unsigned char currByte = unsigned char((currPix >> j) & 0xff);
				(*pixels)[counter] = currByte;
				counter++;
			}
		}

		// Cleanup.
		delete[] buf;
		flip();
	}
	void Image::writeTiff(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
		TIFF* tif = TIFFOpen(filepath.c_str(), "w");
		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, d);

		switch (type)
		{
		case Type::FLOAT:
			TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
			break;
		case Type::USHORT:
			TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
			break;
		default:
			TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
			break;
		}

		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
		TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

		TIFFWriteEncodedStrip(tif, 0,const_cast<void*>(reinterpret_cast<const void*> (pixels)),tsize_t(totalBytes()));
		TIFFClose(tif);
	}

	void Image::readWebp(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {		
		// Validate header.
		std::ifstream ifile(filepath, std::ios::in | std::ios::binary);
		if (!ifile.is_open())
		{
			throw std::exception(("Could not open .webp file: " + filepath).c_str());
		}
		std::vector<uint8_t> data;
		while (ifile.good())
		{
			data.push_back((uint8_t)ifile.get());
		}
		auto sz = std::filesystem::file_size(filepath);

		// Read contents.
		if (!WebPGetInfo(data.data(), sz, &w, &h))
		{
			throw std::exception("Could not parse .webp header");
		}
		*pixels = WebPDecodeRGBA(data.data(), sz, &w, &h);
		d = 4; // in this setup, webp is RGBA by default
    }

	typedef struct MetadataPayload {
		uint8_t* bytes;
		size_t size;
	} MetadataPayload;

	typedef struct Metadata {
		MetadataPayload exif;
		MetadataPayload iccp;
		MetadataPayload xmp;
	} Metadata;

	// Metadata writing.

	enum {
		METADATA_EXIF = (1 << 0),
		METADATA_ICC = (1 << 1),
		METADATA_XMP = (1 << 2),
		METADATA_ALL = METADATA_EXIF | METADATA_ICC | METADATA_XMP
	};

	static const int kChunkHeaderSize = 8;
	static const int kTagSize = 4;

	// Sets 'flag' in 'vp8x_flags' and updates 'metadata_size' with the size of the
	// chunk if there is metadata and 'keep' is true.
	static int UpdateFlagsAndSize(const MetadataPayload* const payload,
		int keep, int flag,
		uint32_t* vp8x_flags, uint64_t* metadata_size) {
		if (keep && payload->bytes != NULL && payload->size > 0) {
			*vp8x_flags |= flag;
			*metadata_size += kChunkHeaderSize + payload->size + (payload->size & 1);
			return 1;
		}
		return 0;
	}

	// Outputs, in little endian, 'num' bytes from 'val' to 'out'.
	static int WriteLE(FILE* const out, uint32_t val, int num) {
		uint8_t buf[4];
		int i;
		for (i = 0; i < num; ++i) {
			buf[i] = (uint8_t)(val & 0xff);
			val >>= 8;
		}
		return (fwrite(buf, num, 1, out) == 1);
	}

	static int WriteLE24(FILE* const out, uint32_t val) {
		return WriteLE(out, val, 3);
	}

	static int WriteLE32(FILE* const out, uint32_t val) {
		return WriteLE(out, val, 4);
	}

	static int WriteMetadataChunk(FILE* const out, const char fourcc[4],
		const MetadataPayload* const payload) {
		const uint8_t zero = 0;
		const size_t need_padding = payload->size & 1;
		int ok = (fwrite(fourcc, kTagSize, 1, out) == 1);
		ok = ok && WriteLE32(out, (uint32_t)payload->size);
		ok = ok && (fwrite(payload->bytes, payload->size, 1, out) == 1);
		return ok && (fwrite(&zero, need_padding, need_padding, out) == need_padding);
	}

	static int WriteWebPWithMetadata(FILE* const out,
		const WebPPicture* const picture,
		const WebPMemoryWriter* const memory_writer,
		const Metadata* const metadata,
		int keep_metadata,
		int* const metadata_written) {
		const char kVP8XHeader[] = "VP8X\x0a\x00\x00\x00";
		const int kAlphaFlag = 0x10;
		const int kEXIFFlag = 0x08;
		const int kICCPFlag = 0x20;
		const int kXMPFlag = 0x04;
		const size_t kRiffHeaderSize = 12;
		const size_t kMaxChunkPayload = ~0 - kChunkHeaderSize - 1;
		const size_t kMinSize = kRiffHeaderSize + kChunkHeaderSize;
		uint32_t flags = 0;
		uint64_t metadata_size = 0;
		const int write_exif = UpdateFlagsAndSize(&metadata->exif,
			!!(keep_metadata & METADATA_EXIF),
			kEXIFFlag, &flags, &metadata_size);
		const int write_iccp = UpdateFlagsAndSize(&metadata->iccp,
			!!(keep_metadata & METADATA_ICC),
			kICCPFlag, &flags, &metadata_size);
		const int write_xmp = UpdateFlagsAndSize(&metadata->xmp,
			!!(keep_metadata & METADATA_XMP),
			kXMPFlag, &flags, &metadata_size);
		uint8_t* webp = memory_writer->mem;
		size_t webp_size = memory_writer->size;

		*metadata_written = 0;

		if (webp_size < kMinSize) return 0;
		if (webp_size - kChunkHeaderSize + metadata_size > kMaxChunkPayload) {
			fprintf(stderr, "Error! Addition of metadata would exceed "
				"container size limit.\n");
			return 0;
		}

		if (metadata_size > 0) {
			const int kVP8XChunkSize = 18;
			const int has_vp8x = !memcmp(webp + kRiffHeaderSize, "VP8X", kTagSize);
			const uint32_t riff_size = (uint32_t)(webp_size - kChunkHeaderSize +
				(has_vp8x ? 0 : kVP8XChunkSize) +
				metadata_size);
			// RIFF
			int ok = (fwrite(webp, kTagSize, 1, out) == 1);
			// RIFF size (file header size is not recorded)
			ok = ok && WriteLE32(out, riff_size);
			webp += kChunkHeaderSize;
			webp_size -= kChunkHeaderSize;
			// WEBP
			ok = ok && (fwrite(webp, kTagSize, 1, out) == 1);
			webp += kTagSize;
			webp_size -= kTagSize;
			if (has_vp8x) {  // update the existing VP8X flags
				webp[kChunkHeaderSize] |= (uint8_t)(flags & 0xff);
				ok = ok && (fwrite(webp, kVP8XChunkSize, 1, out) == 1);
				webp += kVP8XChunkSize;
				webp_size -= kVP8XChunkSize;
			}
			else {
				const int is_lossless = !memcmp(webp, "VP8L", kTagSize);
				if (is_lossless) {
					// Presence of alpha is stored in the 37th bit (29th after the
					// signature) of VP8L data.
					if (webp[kChunkHeaderSize + 4] & (1 << 4)) flags |= kAlphaFlag;
				}
				ok = ok && (fwrite(kVP8XHeader, kChunkHeaderSize, 1, out) == 1);
				ok = ok && WriteLE32(out, flags);
				ok = ok && WriteLE24(out, picture->width - 1);
				ok = ok && WriteLE24(out, picture->height - 1);
			}
			if (write_iccp) {
				ok = ok && WriteMetadataChunk(out, "ICCP", &metadata->iccp);
				*metadata_written |= METADATA_ICC;
			}
			// Image
			ok = ok && (fwrite(webp, webp_size, 1, out) == 1);
			if (write_exif) {
				ok = ok && WriteMetadataChunk(out, "EXIF", &metadata->exif);
				*metadata_written |= METADATA_EXIF;
			}
			if (write_xmp) {
				ok = ok && WriteMetadataChunk(out, "XMP ", &metadata->xmp);
				*metadata_written |= METADATA_XMP;
			}
			return ok;
		}

		// No metadata, just write the original image file.
		return (fwrite(webp, webp_size, 1, out) == 1);
	}


	void Image::writeWebp(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {	
		WebPPicture pic;
		WebPPictureInit(&pic);
		pic.width = w;
		pic.height = h;		
		pic.argb_stride = w * d * byteSize();
		WebPPictureImportRGBA(&pic,pixels,w*d*byteSize());
		if (pic.error_code)
		{
			throw std::exception(("WebPEncode failed. Error code: " + std::to_string((int)pic.error_code)).c_str());
		}

		// Extract a configuration from the packed bits.
		WebPConfig config;
		WebPConfigInit(&config);
		WebPConfigLosslessPreset(&config, 6);
		if (!WebPValidateConfig(&config)) {
			throw std::exception("Error! Invalid configuration.");
		}


		// Encode.
		WebPMemoryWriter memory_writer;
		WebPMemoryWriterInit(&memory_writer);		
		pic.writer = WebPMemoryWrite;
		pic.custom_ptr = &memory_writer;		
		if (!WebPEncode(&config, &pic)) {
			const WebPEncodingError error_code = pic.error_code;
			WebPMemoryWriterClear(&memory_writer);
			WebPPictureFree(&pic);
			if (error_code == VP8_ENC_ERROR_OUT_OF_MEMORY ||
				error_code == VP8_ENC_ERROR_BAD_WRITE) {
				return;
			}
			throw std::exception(("WebPEncode failed. Error code: " + std::to_string((int)error_code)).c_str());
		}

		FILE* out = fopen(filepath.c_str(), "wb");
		Metadata metadata;
		int metadata_written;

		if (!WriteWebPWithMetadata(out, &pic, &memory_writer, &metadata,
				false, &metadata_written)) {
			throw std::exception("Error writing WebP file!\n");		
		}

		WebPMemoryWriterClear(&memory_writer);
		WebPPictureFree(&pic);
		fclose(out);
    }
}