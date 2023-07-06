#pragma once
#include "png.h"

namespace ImageCodecs
{
	class Image
	{
		int h_ = 0;
		int w_ = 0;
		int d_ = 0;
		unsigned char* pixels_ = nullptr;

	public:

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

		~Image() 
		{
			if (pixels_)
			{
				delete[] pixels_;
			}
		}

	private:

	};
}