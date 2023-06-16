#pragma once
#include <string>

namespace ImageCodecs
{
	class BaseCodec
	{
	public:	
		static void read(std::string filename, unsigned char* pixels, int& w, int& h, int& d);
		static void write(std::string filename, unsigned char* pixels, int& w, int& h, int& d);
	};
}