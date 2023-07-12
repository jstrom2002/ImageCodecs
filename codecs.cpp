#include "codecs.h"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iostream>

#define NV_DDS_NO_GL_SUPPORT
#include "nv_dds.h"

#define TJE_IMPLEMENTATION
#include "jpeg_enc.h"
#include "jpeg_dec.h"

#include "png.h"
#include "png_encoder.h"

extern "C" {
	#include <tiffio.h>
}

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

// required libs
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "lzma.lib")
#ifdef _DEBUG
#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "libpng16d.lib")
#pragma comment(lib, "tiffd.lib")
#else
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng16.lib")
#pragma comment(lib, "tiff.lib")
#endif

#include <libpng16/png.h>

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
		else if (ext == ".ppm")
			readPpm(filepath, &pixels_, w_, h_, d_, type_);
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
		else if (ext == ".ppm")
			writePpm(filepath, pixels_, w_, h_, d_, type_);
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

	void Image::transpose(unsigned char* pixels, const int w, const int h, const int d)
	{
		unsigned char* flipped = new unsigned char[w * h * d];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d; ++k)
				{
					unsigned int r = i * w * d;
					unsigned int c = j * d;
					unsigned int r2 = j * h * d;
					unsigned int c2 = i * d;
					flipped[r + c + k] = pixels[r2 + c2 + k];
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < w * h * d; ++i)
		{
			pixels[i] = flipped[i];
		}
		delete[] flipped;
	}


	void Image::flipImage(unsigned char* pixels, const int w, const int h, const int d)
	{
		unsigned char* flipped = new unsigned char[w * h * d];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d; ++k)
				{
					unsigned int r = i * w * d;
					unsigned int r_inv = (h - 1 - i) * w * d;
					unsigned int c = j * d;
					flipped[r + c + k] = pixels[r_inv + c + k];
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < w * h * d; ++i)
		{
			pixels[i] = flipped[i];
		}
		delete[] flipped;
	}

	void Image::swapBR(unsigned char* pixels, const int w, const int h, const int d)
	{
		unsigned char* flipped = new unsigned char[w * h * d];

		// Copy pixels in reverse order.
		for (unsigned int i = 0; i < h; ++i)
		{
			for (unsigned int j = 0; j < w; ++j)
			{
				for (unsigned int k = 0; k < d; ++k)
				{
					unsigned int r = i * w * d;
					unsigned int c = j * d;
					if (k == 0)
					{
						flipped[r + c + 0] = pixels[r + c + 2];
					}
					else if (k == 2)
					{
						flipped[r + c + 2] = pixels[r + c + 0];
					}
					else {
						flipped[r + c + k] = pixels[r + c + k];
					}
				}
			}
		}

		// Now copy back over to original array.
		for (unsigned int i = 0; i < w * h * d; ++i)
		{
			pixels[i] = flipped[i];
		}
		delete[] flipped;
	}

	// Adapted from: https://github.com/marc-q/libbmp/blob/master/CPP/libbmp.cpp
	// NOTE: handles only 3 channel RGB .bmp files with 'BITMAPINFOHEADER' format
	void Image::readBmp(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
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

		std::ifstream f_img(filename.c_str(), std::ios::binary);
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
	void Image::writeBmp(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
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
		std::ofstream f_img(filename.c_str(), std::ios::binary);

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
		int totalBytes = 0;
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
		totalBytes = image.get_size();

		if (image.get_num_mipmaps() > 1) {
			surf = image.get_mipmap(0);
			w = surf.get_width();
			h = surf.get_height();
			d = surf.get_depth();
			totalBytes = surf.get_size();			
		}

		switch (image.get_type()) {
		case nv_dds::TextureType::TextureFlat:
			break;
		case nv_dds::TextureType::TextureCubemap:
			throw std::exception("Cannot handle .dds cubemap textures");
		case nv_dds::TextureType::Texture3D:
			throw std::exception("Cannot handle .dds 3D textures");
		}

		*pixels = new unsigned char[w*h*d];
		unsigned int byte_counter = 0;		
		if (image.get_num_mipmaps() > 1) 
		{
			memcpy(*pixels, surf, w*h*d);
		}
		else
		{			
			memcpy(*pixels, image, w*h*d);
		}

		flipImage(*pixels, w, h, d);

		surf.clear();
		image.clear();		
	}
	void Image::writeDds(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		nv_dds::CTexture img;
		img.create(w,h,1,w*h*d,pixels);	
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
		ddsimage.save(filename);		
		img.clear();
		ddsimage.clear();
	}

	void Image::readExr(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		std::vector<unsigned char> loadedData;
		std::ifstream ifile(filename.c_str(), std::ios::in | std::ios::binary);
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
			d = 4;
			*pixels = new unsigned char[w * h * 4 * sizeof(float)];
			memcpy(*pixels, floatPixels, w * h * 4 * sizeof(float));
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

	void Image::writeExr(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		// TO DO: finish
		//auto ret = SaveEXRImageToFile(&exr_image, &exr_header, outfilename, &err);
	}

	void Image::readGif(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
	}
	void Image::writeGif(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
	}

	void Image::writeHdr(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		// TO DO: finish
	}


	typedef unsigned char RGBE[4];
#define R			0
#define G			1
#define B			2
#define E			3

#define  MINELEN	8				// minimum scanline length for encoding
#define  MAXELEN	0x7fff			// maximum scanline length for encoding
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
			cols += 3;
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
		d_ = 3; // All .hdr files are RGB

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
		float* floatPixels = new float[w * h * 3];

		RGBE* scanline = new RGBE[w];
		if (!scanline) {
			fclose(file);
			throw std::exception("Invalid file format");
		}

		// convert image and copy over float data.
		type = Type::FLOAT;
		*pixels = new unsigned char[w * h * 3 * sizeof(float)];
		unsigned int lineCount = 0;
		for (int y = h - 1; y >= 0; y--) {
			if (decrunchHDR(scanline, w, file) == false)
				break;
			workOnRGBE(scanline, w, floatPixels);
			memcpy(*pixels + lineCount, floatPixels, w * 3 * sizeof(float));
			floatPixels += w * 3;
			lineCount += w * 3 * sizeof(float);
		}
		
		// Cleanup.
		delete[] scanline;
		fclose(file);
	}

	void Image::readJpg(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
	{
		FILE* f = fopen(filename.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		int size = (int)ftell(f);
		(*pixels) = new unsigned char[size];
		fseek(f, 0, SEEK_SET);
		size = (int)fread((*pixels), 1, size, f);
		fclose(f);

		njInit();
		if (njDecode((*pixels), size)) {
			delete[] (*pixels);
			throw std::exception("Error decoding the input file.\n");
		}

		d = 3;
		w = njGetWidth();
		h = njGetHeight();

		njDone();
	}

	void Image::writeJpg(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		//tje_encode_to_file(filename.c_str(), w, h, d, pixels);
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
		*pixels = new unsigned char[w * h * d];
		memcpy(*pixels, img_data, w*h*d);
	}

	void Image::writePng(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
	{
		png_encoder::saveToFile(filepath,pixels,w,h,d);
	}

    void Image::readPpm(std::string filepath, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {
        std::ifstream infile(filepath, std::ifstream::binary);
        if (!infile.is_open())
        {
            throw std::exception(("Failed to open " + filepath).c_str());
        }

        int channels = 3;
        std::string mMagic;
        infile >> mMagic;
        infile.seekg(1, infile.cur);
        char c;
        infile.get(c);
        if (c == '#')
        {
            // We got comments in the PPM image and skip the comments
            while (c != '\n')
            {
                infile.get(c);
            }
        }
        else
        {
            infile.seekg(-1, infile.cur);
        }

        int maxPixelValue;
        infile >> w >> h >> maxPixelValue;
        if (maxPixelValue != 255)
        {
            throw std::exception("Max value for pixel data should be 255");
        }

        (*pixels) = new uint8_t[w * h * 3];

        // ASCII
        if (mMagic == "P3")
        {
            for (int i = 0; i < w * h * 3; i++)
            {
                std::string pixel_str;
                infile >> pixel_str;
                (*pixels)[i] = static_cast<uint8_t> (std::stoi(pixel_str));
            }
        }
        // Binary
        else if (mMagic == "P6")
        {
            // Move curser once to skip '\n'
            infile.seekg(1, infile.cur);
            infile.read(reinterpret_cast<char*>((*pixels)), w * h * 3);
        }
        else // Unrecognized format
        {
            throw std::exception("Unrecognized .ppm magic value. Should be either P3 (ascii) or P6 (binary) data");
        }
    }

    void Image::writePpm(std::string filepath, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
        std::ofstream outfile(filepath, std::ofstream::binary);
        if (outfile.fail())
        {
            throw std::exception("Failed to write ");
        }
        outfile << "P6" << "\n" << w << " " << h << "\n" << 255 << "\n";
        outfile.write(reinterpret_cast<char*>(pixels), w * h * 3);
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
		size_t FileSize = 0;
		File.seekg(0, std::ios_base::end);
		FileSize = File.tellg();
		File.seekg(0, std::ios_base::beg);
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
					std::copy(&Buffer[0], &Buffer[ImageSize], &(*pixels)[0]);
					for (size_t i = 0; i < ImageSize; i += PixelSize)
						std::swap(*pixels[i], *pixels[i + 2]);
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
			flipImage(*pixels, w, h, d); // for some reason, this code loads the .tga data upside down
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

	void Image::readTiff(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {
		TIFFSetWarningHandler(0);
		TIFF* tif = TIFFOpen(filename.c_str(), "r");
		tdata_t* buf;
		tsize_t scanline;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		scanline = TIFFScanlineSize(tif);
		buf = (tdata_t*)_TIFFmalloc(scanline);
		*pixels = new unsigned char[w*h*d];
		unsigned int counter = 0;
		for (uintmax_t row = 0; row < h; row++)
		{
			int n = TIFFReadScanline(tif, buf, row, 0);
			if (n == -1) {
				printf("Error");
				break;;
			}
			for (unsigned int i = 0; i < scanline; ++i)
			{
				auto c = unsigned char((uintmax_t)buf[i] / std::numeric_limits<uintmax_t>::max());
				(*pixels)[counter] = c;
				counter++;
			}
		}
	}
	void Image::writeTiff(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
		//// TO DO: finish debugging

		//TIFF* image = TIFFOpen("test\\test_icdTest.tif","w"); //filename.c_str(), "w");
		//TIFFSetField(image, TIFFTAG_IMAGEWIDTH, w);
		//TIFFSetField(image, TIFFTAG_IMAGELENGTH, h);
		//TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 32);
		//TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
		//TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);
		//TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		//TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		//TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
		//TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
		//TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);		
		//unsigned char* scan_line = new unsigned char[w * d];
		//for (int i = 0; i < h; i++) {
		//	memcpy(scan_line, (void*)pixels[i * w], w * d);
		//	TIFFWriteScanline(image, scan_line, i, 0);
		//}
		//
		//size_t linebytes = d * w;
		//unsigned char* buf = nullptr;
		//if (TIFFScanlineSize(image) > linebytes)
		//	buf = new unsigned char[linebytes];
		//else
		//	buf = new unsigned char[TIFFScanlineSize(image)];
		//TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(image, w * d));
		//for (uintmax_t row = 0; row < h; row++){
		//	//memcpy(buf, &image[(h - row - 1) * linebytes], linebytes);    // check the index here, and figure out why not using h*linebytes
		//	if (TIFFWriteScanline(image, buf, row, 0) < 0)
		//		break;
		//}
		//TIFFClose(image);
		//delete[] scan_line;
    }

	void Image::readWebp(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type)
    {
    }
	void Image::writeWebp(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type)
    {
    }
}