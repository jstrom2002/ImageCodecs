// Include opencv4 to display images
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "libsharpyuv.lib")
#ifdef NDEBUG
#pragma comment(lib, "opencv_core4.lib")
#pragma comment(lib, "opencv_highgui4.lib")
#pragma comment(lib, "opencv_imgcodecs4.lib")
#pragma comment(lib, "opencv_imgproc4.lib")
#else
#pragma comment(lib, "opencv_core4d.lib")
#pragma comment(lib, "opencv_highgui4d.lib")
#pragma comment(lib, "opencv_imgcodecs4d.lib")
#pragma comment(lib, "opencv_imgproc4d.lib")
#endif

#include "codecs.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
	if (!std::filesystem::exists("test"))
	{
		std::filesystem::create_directory("test");
	}

	for (auto& testFile : std::filesystem::recursive_directory_iterator("data"))
	{
		try
		{
			ImageCodecs::Image img;

			// Test .read()
			std::cout << "reading from disk: " << testFile.path().string() << std::endl;
			img.read(testFile.path().string());

			// Display read pixels.
			if (!img.empty())
			{
				int typ = img.channels() == 3 ? CV_8UC3 : CV_8UC4;
				if (img.type() == ImageCodecs::Type::FLOAT)
				{
					typ = img.channels() == 3 ? CV_32FC3 : CV_32FC4;
				}
				cv::Mat displayImg = cv::Mat::zeros(img.rows(), img.cols(), typ);
				memcpy(displayImg.data, (*img.data()), img.cols() * img.rows() * img.channels());
				if (img.type() == ImageCodecs::Type::FLOAT)
				{
					auto mn = cv::mean(displayImg);
					std::cout << "mean: " << mn << std::endl;
					displayImg.convertTo(displayImg, CV_8UC3, 255.0);
				}
				cv::imshow(testFile.path().extension().string(), displayImg);
				cv::waitKey();
				displayImg.deallocate();
			}

			// Test .write()
			auto parent = testFile.path().parent_path();
			auto newName = testFile.path().filename().string();
			auto ext = testFile.path().extension().string();
			newName = "test\\" + newName.substr(0, newName.rfind(".")) + "_icdTest" + ext;
			img.write(newName);
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	return EXIT_SUCCESS;
}