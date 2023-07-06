#include <cmath>
#include <exception>
#include <string>
#include <vector>

namespace ImageCodecs
{
	void flip(unsigned char** pixels, const int w, const int h, const int d);

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
}