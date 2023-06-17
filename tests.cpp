#include "Image.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
	for (auto& testFile : std::filesystem::recursive_directory_iterator("data"))
	{
		try 
		{
			ImageCodecs::Image img;

			// Test .read()
			std::cout << "reading from disk: " << testFile.path().string() << std::endl;
			img.read(testFile.path().string());

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