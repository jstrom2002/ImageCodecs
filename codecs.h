#include <cmath>
#include <exception>
#include <string>
#include <vector>

namespace ImageCodecs
{
	class Image
	{
		int h_ = 0;
		int w_ = 0;
		int d_ = 0;
		unsigned char* pixels_ = nullptr;

		void flipImage(unsigned char* pixels, const int w, const int h, const int d);

		// codecs per filetype:
		void readBmp(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeBmp(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readDds(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeDds(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readGif(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeGif(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readHdr(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeHdr(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readJpg(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeJpg(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readPng(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writePng(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readPpm(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writePpm(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readTga(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeTga(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readTiff(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeTiff(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

		void readWebp(std::string filename, unsigned char** pixels, int& w, int& h, int& d);
		void writeWebp(std::string filename, unsigned char* pixels, int& w, int& h, int& d);

	public:

		// row-major index access for contiguous array of pixel data.
		inline unsigned char idx(int i, int j, int k)
		{
			return pixels_[i * w_ * d_ + j * d_ + k];
		}
		inline int channels() { return d_; }
		inline int cols() { return w_; }
		inline int rows() { return h_; }
		inline void load(unsigned char* pixels, int w, int h, int channels)
		{
			d_ = channels;
			w_ = w;
			h_ = h;
			pixels_ = pixels;
		}
		void read(std::string filepath);
		void write(std::string filepath);
		inline void flip(){flipImage(pixels_, w_, h_, d_);}
		~Image(){delete[] pixels_;}
	};
}