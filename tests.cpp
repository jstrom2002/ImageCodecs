#include "bmp.h"
#include "ppm.h"
#include "tga.h"

#include <iostream>


bool testBMP()
{
	return false;
}

bool testPNG()
{
	return false;

}

bool testPPM()
{
	return false;

}

bool testTGA()
{
	return false;

}

bool runAllTests()
{
	bool testsSucceeded = true;
	testsSucceeded &= testBMP();
	testsSucceeded &= testPNG();
	testsSucceeded &= testPPM();
	testsSucceeded &= testTGA();

	return testsSucceeded;
}

int main(int argc, char** argv)
{
	bool testsSucceeded = runAllTests();

	return EXIT_SUCCESS;
}