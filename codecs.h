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
		USHORT,
		FLOAT
	};

	class Image
	{
		const int USHORT_SIZE = 2; // this lib requires the size of all 'ushort' types == 2 bytes, else many decoders will not work.
		const int FLOAT_SIZE = 4; // this lib requires the size of all 'float' types == 4 bytes, else many decoders will not work.
		int h_ = 0;
		int w_ = 0;
		int d_ = 0;
		unsigned char* pixels_ = nullptr;
		Type type_ = Type::UBYTE;
		
		inline int byteSize(Type type)
		{
			if (type_ == Type::FLOAT)
				return FLOAT_SIZE;
			else if (type_ == Type::USHORT)
				return USHORT_SIZE;
			else
				return 1;
		}
		void flip(unsigned char* pixels, const int w, const int h, const int d, const Type& type);
		void swapBR(unsigned char* pixels, const int w, const int h, const int d, const Type& type);
		void transpose(unsigned char* pixels, const int w, const int h, const int d, const Type& type);

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
		void readPbm(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writePbm(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readTga(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeTga(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readTiff(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeTiff(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

		void readWebp(std::string filename, unsigned char** pixels, int& w, int& h, int& d, Type& type);
		void writeWebp(std::string filename, unsigned char* pixels, int& w, int& h, int& d, Type& type);

	public:
		inline int byteSize() { return byteSize(type_); }
		inline int channels() { return d_; }
		inline int cols() { return w_; }
		inline unsigned char** data() { return &pixels_; }
		inline bool empty() { return h_ == 0 || w_ == 0 || d_ == 0 || pixels_ == nullptr; }
		inline void flip() { flip(pixels_, w_, h_, d_, type_); }
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
		inline void swapBR(){swapBR(pixels_, w_, h_, d_, type_);}
		inline int totalBytes() { return w_ * h_ * d_ * byteSize(); }
		inline Type type() { return type_; }
		void write(std::string filepath);
		~Image(){delete[] pixels_;}
	};
}