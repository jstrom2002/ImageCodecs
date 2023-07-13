#pragma once
#include <cmath>
#include <exception>
#include <string>
#include <vector>

namespace ImageCodecs
{
	enum class Type
	{
		UBYTE,
		FLOAT
	};

	class Image
	{
		int h_ = 0;
		int w_ = 0;
		int d_ = 0;
		unsigned char* pixels_ = nullptr;
		Type type_ = Type::UBYTE;
		
		void flipImage(unsigned char* pixels, const int w, const int h, const int d);
		void swapBR(unsigned char* pixels, const int w, const int h, const int d);
		void transpose(unsigned char* pixels, const int w, const int h, const int d);

		// codecs per filetype:
		void readBmp(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeBmp(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readDds(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeDds(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readExr(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeExr(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readGif(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeGif(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readHdr(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeHdr(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readJpg(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeJpg(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readPng(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writePng(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		// NOTE: works for all netpnm types: pbm,pfm,pgm,ppm,pnm
		void readPpm(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writePpm(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readTga(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeTga(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readTiff(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeTiff(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readWebp(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeWebp(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

	public:
		inline int channels() { return d_; }
		inline int cols() { return w_; }
		inline unsigned char** data() { return &pixels_; }
		inline bool empty() { return h_ == 0 || w_ == 0 || d_ == 0 || pixels_ == nullptr; }
		inline void flip() { flipImage(pixels_, w_, h_, d_); }
		// row-major index access for contiguous array of pixel data.
		template <typename T>
		inline T idx(int i, int j, int k)
		{
			T ret;
			memcpy(&T, pixels_ + (i * w_ * d_ * sizeof(T) + j * d_ * sizeof(T) + k * sizeof(T)), sizeof(T));
			return ret;
		}
		inline void load(unsigned char* pixels, int w, int h, int channels)
		{
			d_ = channels;
			w_ = w;
			h_ = h;
			pixels_ = pixels;
		}
		void read(std::string filepath);
		inline int rows() { return h_; }
		inline void swapBR(){swapBR(pixels_, w_, h_, d_);}
		inline Type type() { return type_; }
		void write(std::string filepath);
		~Image(){delete[] pixels_;}
	};
}