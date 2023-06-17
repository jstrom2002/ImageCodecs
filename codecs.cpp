#include "codecs.h"
#include "png.h"
#include <exception>
#include <fstream>
#include <filesystem>

#define NV_DDS_NO_GL_SUPPORT
#include "nv_dds.h"

#define TJE_IMPLEMENTATION
#include "jpeg_enc.h"
#include "jpeg_dec.h"

namespace ImageCodecs
{
	void readBmp(std::string filename, unsigned char** pixels, int& w, int& h, int& d)
	{
		const int bitDepth = 24; // For now, pixel depths are hardcoded to 8-bit RGB
		int headerSize = 0;//size of header before pixel array
		int hgt = 0;//gives image width
		int horizontalResolution = 0;
		int imageSize = 0;//gives image size
		//const int numberOfColorsInPalatte = 0;//colors in palatte, 0 is default (2^n colors). No other values are handled for this class currently.
		int numberOfPixels = 0;//number of total pixel vectors, ie for 24-bit arrays it would be 3 * wdt * hgt
		int pixelsPerRow = 0;//number of pixels in a row, width*3
		int padBytes = 0;//number of bytes padding each row
		int rowSize = 0;//number of bytes necessary to store one row
		int verticalResolution = 0; // resolutions should be either 96 or 72 dpi 
		int wdt = 0;//gives image height 
		const std::string headertype = "BITMAPINFOHEADER";
		std::string colorOrdering = "BGR";//displays whether the array values are RGB, BGR, grayscale, etc
		std::vector<unsigned char> header;//byte vector of the data for the header to BMP file
		std::vector<std::vector<unsigned char>> pixels_;//an array of all the pixels in the BMP file 

		//open BMP file
		FILE* f = fopen(filename.c_str(), "rb");

		//read preliminary file data -- 14 bytes
		unsigned char prelimData[14];
		fread(prelimData, sizeof(unsigned char), 14, f);
		for (int i = 0; i < 14; ++i)
		{
			header.push_back(prelimData[i]);
		}
		int numberOfBytes = (header[5] << 24) ^ (header[4] << 16) ^ (header[3] << 8) ^ header[2];//read number of bytes in file
		int reservedBytes = (header[9] << 24) ^ (header[8] << 16) ^ (header[7] << 8) ^ header[6];//read reserved data -- unused
		headerSize = (header[13] << 24) ^ (header[12] << 16) ^ (header[11] << 8) ^ header[10];//read starting address
		if (headerSize != 54)
		{
			throw std::exception("Headers with non-54 byte length are not supported.");
		}
		std::string headertype2;
		switch (headerSize)
		{
		case 12 + 14:  headertype2 = "BITMAPCOREHEADER"; break;
		case 64 + 14:  headertype2 = "OS22XBITMAPHEADER"; break;
		case 16 + 14:  headertype2 = "OS22XBITMAPHEADER"; break;
		case 40 + 14:  headertype2 = "BITMAPINFOHEADER"; break;
		case 52 + 14:  headertype2 = "BITMAPV2INFOHEADER"; break;
		case 56 + 14:  headertype2 = "BITMAPV3INFOHEADER"; break;
		case 10 + 14:  headertype2 = "BITMAPV4HEADER "; break;
		case 124 + 14: headertype2 = "BITMAPV5HEADER"; break;
		}

		if (headertype2 != "BITMAPINFOHEADER")
		{
			throw std::exception("No headers but BITMAPINFOHEADER type are supported.");
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
			throw std::exception("Non 8-bit pixel depth is unimplemented");
		}
		int compressionMethod = (header[33] << 24) ^ (header[32] << 16) ^ (header[31] << 8) ^ header[30];//tells what method of compression is used (0 = uncompressed) 
		if (compressionMethod != 0)
		{
			throw std::exception("Decompression is unimplemented");
		}
		int numberOfColorsInPalatte = (header[49] << 24) ^ (header[48] << 16) ^ (header[47] << 8) ^ header[46];//gives number of colors in color palatte, 0 = 2^n (default)
		if (numberOfColorsInPalatte != 0)
		{
			throw std::exception("Color palettes != 2^n are unimplemented");
		}

		// Update header
		rowSize = std::floor(((bitDepth * wdt) + 31.0) / 32.0) * 4;
		numberOfBytes = headerSize + (rowSize * hgt);
		int temp = verticalResolution;
		verticalResolution = horizontalResolution;
		horizontalResolution = temp;
		imageSize = hgt * rowSize;
		numberOfPixels = 3 * hgt * wdt;
		pixelsPerRow = 3 * wdt;
		padBytes = rowSize - pixelsPerRow;


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
					if (i > 0 && i % 3 == 0)
					{

						pixels_.push_back(tempVec);
						tempVec.clear();
					}
					tempVec.push_back(data[i]);
				}
			}
		}
		else
		{
			throw std::exception("Cannot handle non-RGB .bmp files");
		}

		// Output to pixels array.		
		(*pixels) = new unsigned char[pixels_[0].size() * pixels_.size()];
		for (unsigned int i = 0; i < pixels_.size(); ++i)
		{
			for (unsigned int j = 0; j < pixels_[i].size(); ++j)
			{
				(*pixels)[i * pixels_[0].size() + j] = pixels_[i][j];
			}
		}
		pixels_.clear();
	}

	void writeBmp(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
	{
		FILE* f = fopen(filename.c_str(), "wb"); //write file

		const int bitDepth = d * 8;// bits per pixel
		int pixelsPerRow = d * w;
		int rowSize = std::floor(((bitDepth * w) + 31.0) / 32.0) * 4;
		int padBytes = rowSize - pixelsPerRow;
		int wRes = 2048;
		int hRes = 2048;

		//write header
		std::vector<unsigned char> header = std::vector<unsigned char>
		{
				0x42, 0x4D, 0x26, 0x4F, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
				0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x8B, 0x01, 0x00, 0x00, 0x5C, 0x01,
				0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x4E,
				0x06, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		header[18] = w >> 0;
		header[19] = w >> 8;
		header[20] = w >> 16;
		header[21] = w >> 24;

		header[22] = h >> 0;
		header[23] = h >> 8;
		header[24] = h >> 16;
		header[25] = h >> 24;

		header[38] = wRes >> 0;
		header[39] = wRes >> 8;
		header[40] = wRes >> 16;
		header[41] = wRes >> 24;

		header[42] = hRes >> 0;
		header[43] = hRes >> 8;
		header[44] = hRes >> 16;
		header[45] = hRes >> 24;

		fwrite(header.data(), sizeof(unsigned char), 54, f);

		//write pixel data
		int pixelCount = 0;
		std::vector<unsigned char>row(rowSize, 0);
		while (pixelCount < w * h) 
		{
			for (int i = 0; i < rowSize; i += 3) 
			{
				if (padBytes == 0 || (i < pixelsPerRow && pixelCount < w * h * d)) 
				{
					row[i + 0] = pixels[pixelCount + 0];
					row[i + 1] = pixels[pixelCount + 1];
					row[i + 2] = pixels[pixelCount + 2];
					++pixelCount;
				}
				else 
				{
					while (i < rowSize) 
					{
						row[i + 0] = 0;
						++i;
					}
				}
			}
			fwrite(row.data(), sizeof(unsigned char), rowSize, f);
		}

		fclose(f);
	}

	void readDds(std::string filepath, unsigned char** pixels, int& w, int& h, int& d)
	{
		nv_dds::CDDSImage image;
		nv_dds::CSurface surf;
		int totalBytes = 0;
		try
		{
			bool flipImage = false;
			image.load(filepath, flipImage);
		}
		catch (std::exception e1)
		{
			throw std::exception(e1.what());
		}

		w = image.get_width();
		h = image.get_height();
		d = image.get_depth();
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

		(*pixels) = new unsigned char[totalBytes];
		unsigned int byte_counter = 0;
		if (image.get_num_mipmaps() > 1) 
		{
			memcpy((*pixels), surf, surf.get_size());
		}
		else
		{
			memcpy((*pixels), image, image.get_size());
		}

		surf.clear();
		image.clear();
	}
	void writeDds(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
	{
		nv_dds::CTexture img;
		img.create(w,h,d,w*h*d,pixels);
		nv_dds::CDDSImage ddsimage;
		ddsimage.create_textureFlat(d == 3 ? GL_RGB : GL_RGBA, d, img);
		ddsimage.save(filename);
		img.clear();
		ddsimage.clear();
	}

	void readGif(std::string filename, unsigned char** pixels, int& w, int& h, int& d)
	{
	}
	void writeGif(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
	{
	}

	void writeHdr(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
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


	void readHdr(std::string filepath, unsigned char** pixels, int& w_, int& h_, int& d_)
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

		// convert image 
		for (int y = h - 1; y >= 0; y--) {
			if (decrunchHDR(scanline, w, file) == false)
				break;
			workOnRGBE(scanline, w, floatPixels);
			floatPixels += w * 3;
		}

		delete[] scanline;
		fclose(file);
	}

	void readJpg(std::string filename, unsigned char** pixels, int& w, int& h, int& d)
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

	void writeJpg(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
	{
		//tje_encode_to_file(filename.c_str(), w, h, d, pixels);
	}

	void readPng(std::string filepath, unsigned char** pixels, int& w, int& h, int& d)
	{
		std::vector<uint8_t> px;
		uint32_t w_, h_, ch;
		d = 4; // default load as RGBA data

		fpng_decode_file(filepath.c_str(), px, w_, h_, ch, d);
		w = w_;
		h = h_;

		(*pixels) = new unsigned char[w * h * d];
		memcpy((*pixels), px.data(), px.size());
	}

	void writePng(std::string filepath, unsigned char* pixels, int& w, int& h, int& d)
	{
		fpng_encode_image_to_file(filepath.c_str(), pixels, w, h, d);
	}

    void readPpm(std::string filepath, unsigned char** pixels, int& w, int& h, int& d)
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

    void writePpm(std::string filepath, unsigned char* pixels, int& w, int& h, int& d)
    {
        std::ofstream outfile(filepath, std::ofstream::binary);
        if (outfile.fail())
        {
            throw std::exception("Failed to write ");
        }
        outfile << "P6" << "\n" << w << " " << h << "\n" << 255 << "\n";
        outfile.write(reinterpret_cast<char*>(pixels), w * h * 3);
    }

    void readTga(std::string filepath, unsigned char** pixels, int& w, int& h, int& d)
    {
        std::fstream hFile(filepath, std::ios::in | std::ios::binary);
        if (!hFile.is_open())
        {
            throw std::invalid_argument("File Not Found.");
        }

        std::uint8_t Header[18] = { 0 };
        static std::uint8_t DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
        static std::uint8_t IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

        hFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));
        if (!std::memcmp(DeCompressed, &Header, sizeof(DeCompressed)))
        {
            int BitsPerPixel = Header[16];
            w = Header[13] * 256 + Header[12];
            h = Header[15] * 256 + Header[14];
            d = BitsPerPixel / 8;
            int size = ((w * BitsPerPixel + 31) / 32) * 4 * h;

            (*pixels) = new unsigned char(size);
            bool ImageCompressed = false;
            hFile.read(reinterpret_cast<char*>((*pixels)), size);
        }
        else if (!std::memcmp(IsCompressed, &Header, sizeof(IsCompressed)))
        {
            throw std::exception("Error! Cannot read compressed .tga files");
        }
        else
        {
            throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit TGA File.");
        }
    }

    void writeTga(std::string filepath, unsigned char* pixels, int& w, int& h, int& d)
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

	void readTiff(std::string filename, unsigned char** pixels, int& w, int& h, int& d)
    {
    }
	void writeTiff(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
    {
    }

	void readWebp(std::string filename, unsigned char** pixels, int& w, int& h, int& d)
    {
    }
	void writeWebp(std::string filename, unsigned char* pixels, int& w, int& h, int& d)
    {
    }
}