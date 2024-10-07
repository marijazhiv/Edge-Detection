#include <iostream>
#include <stdlib.h>
#include "BitmapRawConverter.h"
#include <tbb/blocked_range.h>
#include "tbb/task_group.h"
#include "tbb/tick_count.h"

#define __ARG_NUM__				6
#define FILTER_SIZE				5
#define THRESHOLD				128

using namespace std;
using namespace tbb;

unsigned int grain_size;

// Prewitt operators
int* filterHor = new int[FILTER_SIZE * FILTER_SIZE];
int* filterVer = new int[FILTER_SIZE * FILTER_SIZE];


void generateFilter(int i, int* filterHor, int* filterVer)
{

	int start = 0 - i / 2;
	int start2 = start;
	int p = 0;
	for (int k = start; k <= i / 2; k++)
	{
		for (int j = start; j <= i / 2; j++)
		{
			filterHor[p] = j;
			filterVer[p] = k;
			p++;
		}
	}

}

/**
* @brief Serial version of edge detection algorithm implementation using Prewitt operator
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_prewitt(int* inBuffer, int* outBuffer, int width, int height)  //TODO obrisati
{

	for (int i = FILTER_SIZE / 2; i < width - FILTER_SIZE / 2; i++) {
		for (int j = FILTER_SIZE / 2; j < height - FILTER_SIZE / 2; j++) {
			int Gx = 0;
			int Gy = 0;
			int G = 0;
			for (int k = 0; k < FILTER_SIZE * FILTER_SIZE; k++) {
				Gx += inBuffer[(j + filterHor[k]) * width + (i + filterVer[k])] * filterHor[k];			//horizontalna komponentna
				Gy += inBuffer[(j + filterHor[k]) * width + (i + filterVer[k])] * filterVer[k];			//vertikalna komponenta
			}

			G = abs(Gx) + abs(Gy);                                            
			outBuffer[j * width + i] = (G < THRESHOLD) ? 0 : 255;						//0 (crna boja), 255 (bela)
		}																
	}

}


/**
* @brief Parallel version of edge detection algorithm implementation using Prewitt operator
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_prewitt(int* inBuffer, int* outBuffer, int width, int height, int wBegin, int hBegin, int W, int H)
{
	task_group g;
	if (width > grain_size) {
		g.run([&] { filter_parallel_prewitt(inBuffer, outBuffer, width / 2, height / 2, wBegin, hBegin, W, H); });
		g.run([&] { filter_parallel_prewitt(inBuffer, outBuffer, width / 2, height / 2, wBegin + width / 2, hBegin, W, H); });
		g.run([&] { filter_parallel_prewitt(inBuffer, outBuffer, width / 2, height / 2, wBegin, hBegin + height / 2, W, H); });
		g.run([&] { filter_parallel_prewitt(inBuffer, outBuffer, width / 2, height / 2, wBegin + width / 2, hBegin + height / 2, W, H); });
		g.wait();

	}
	else {																								//serijsko izvrsavanje
		int _wBegin = (wBegin == 0) ? FILTER_SIZE / 2 : wBegin;
		int _wEnd = (wBegin + width == W) ? wBegin + width - FILTER_SIZE / 2 : wBegin + width;
		int _hBegin = (hBegin == 0) ? FILTER_SIZE / 2 : hBegin;
		int _hEnd = (hBegin + height == H) ? hBegin + height - FILTER_SIZE / 2 : hBegin + height;

		for (int i = _wBegin; i < _wEnd; i++) {
			for (int j = _hBegin; j < _hEnd; j++) {
				int Gx = 0;
				int Gy = 0;
				int G = 0;
				for (int k = 0; k < FILTER_SIZE * FILTER_SIZE; k++) {
					Gx += inBuffer[(j + filterHor[k]) * W + (i + filterVer[k])] * filterHor[k];
					Gy += inBuffer[(j + filterHor[k]) * W + (i + filterVer[k])] * filterVer[k];
				}

				G = abs(Gx) + abs(Gy);
				outBuffer[j * W + i] = (G < THRESHOLD) ? 0 : 255;
			}
		}
	}
}

/**
* @brief Serial version of edge detection algorithm
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_edge_detection(int* inBuffer, int* outBuffer, int* midBuffer, int width, int height)	//TODO obrisati
{

	//crno-bela slika
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			midBuffer[j * width + i] = (inBuffer[j * width + i] < THRESHOLD) ? 0 : 255;
		}
	}

	//na crno-beloj slici gledaj okolinu svakog piksela
	for (int i = FILTER_SIZE / 2; i < width - FILTER_SIZE / 2; i++) {
		for (int j = FILTER_SIZE / 2; j < height - FILTER_SIZE / 2; j++) {
			int sum = 0;
			for (int k = 0; k < FILTER_SIZE * FILTER_SIZE; k++) {  //saberi piksele oko piksela (velicine filtera)
				sum += midBuffer[(j + filterHor[k]) * width + (i + filterVer[k])];
			}

			int P = (sum == 0) ? 0 : 1;  //ako su sve beli onda je 0, inace 1
			int O = (sum == 255 * FILTER_SIZE * FILTER_SIZE) ? 1 : 0; //ako su sve crni onda je 1, inace 0

			outBuffer[j * width + i] = (P == O) ? 0 : 255;  //ako je razlika jednaka 0 onda je 0, inace 255
		}
	}



}

/**
* @brief Parallel version of edge detection algorithm
*
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_edge_detection(int* inBuffer, int* outBuffer, int* midBuffer, int width, int height, int wBegin, int hBegin, int W, int H)
{

	task_group g;
	if (width > grain_size) {
		g.run([&] { filter_parallel_edge_detection(inBuffer, outBuffer, midBuffer, width / 2, height / 2, wBegin, hBegin, W, H); });
		g.run([&] { filter_parallel_edge_detection(inBuffer, outBuffer, midBuffer, width / 2, height / 2, wBegin + width / 2, hBegin, W, H); });
		g.run([&] { filter_parallel_edge_detection(inBuffer, outBuffer, midBuffer, width / 2, height / 2, wBegin, hBegin + height / 2, W, H); });
		g.run([&] { filter_parallel_edge_detection(inBuffer, outBuffer, midBuffer, width / 2, height / 2, wBegin + width / 2, hBegin + height / 2, W, H); });
		g.wait();

	}
	else {
		int _wBegin = (wBegin == 0) ? FILTER_SIZE / 2 : wBegin - FILTER_SIZE / 2;
		int _wEnd = (wBegin + width == W) ? wBegin + width - FILTER_SIZE / 2 : wBegin + width + FILTER_SIZE / 2;
		int _hBegin = (hBegin == 0) ? FILTER_SIZE / 2 : hBegin - FILTER_SIZE / 2;
		int _hEnd = (hBegin + height == H) ? hBegin + height - FILTER_SIZE / 2 : hBegin + height + FILTER_SIZE / 2;

		//crno-bela slika
		for (int i = wBegin; i < wBegin + width; i++) {
			for (int j = hBegin; j < hBegin + height; j++) {
				midBuffer[j * W + i] = (inBuffer[j * W + i] < THRESHOLD) ? 0 : 255;
			}
		}

		
		for (int i = _wBegin; i < _wEnd; i++) {
			for (int j = _hBegin; j < _hEnd; j++) {
				int sum = 0;
				for (int k = 0; k < FILTER_SIZE * FILTER_SIZE; k++) { 
					sum += midBuffer[(j + filterHor[k]) * W + (i + filterVer[k])];
				}

				int P = (sum == 0) ? 0 : 1;  
				int O = (sum == 255 * FILTER_SIZE * FILTER_SIZE) ? 1 : 0; 

				outBuffer[j * W + i] = (P == O) ? 0 : 255;  
			}
		}
	}
}

/**
* @brief Function for running test.
*
* @param testNr test identification, 1: for serial version, 2: for parallel version
* @param ioFile input/output file, firstly it's holding buffer from input image and than to hold filtered data
* @param outFileName output file name
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/


void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, int* midBuffer, unsigned int width, unsigned int height)
{

	// TODO: start measure
	tick_count startTime = tick_count::now();

	switch (testNr)
	{
	case 1:
		cout << "Running serial version of edge detection using Prewitt operator" << endl;
		filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
		break;
	case 2:
		cout << "Running parallel version of edge detection using Prewitt operator" << endl;
		filter_parallel_prewitt(ioFile->getBuffer(), outBuffer, width, height, 0, 0, width, height);
		break;
	case 3:
		cout << "Running serial version of edge detection" << endl;
		filter_serial_edge_detection(ioFile->getBuffer(), outBuffer, midBuffer, width, height);
		break;
	case 4:
		cout << "Running parallel version of edge detection" << endl;
		filter_parallel_edge_detection(ioFile->getBuffer(), outBuffer, midBuffer, width, height, 0, 0, width, height);
		break;
	default:
		cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
		break;
	}
	// TODO: end measure and display time

	tick_count endTime = tick_count::now();
	cout << "Time: \t\t\t" << (endTime - startTime).seconds() << " seconds\n";


	ioFile->setBuffer(outBuffer);
	ioFile->pixelsToBitmap(outFileName);
}

/**
* @brief Print program usage.
*/
void usage()
{
	cout << "\n\ERROR: call program like: " << endl << endl;
	cout << "ProjekatPP.exe";
	cout << " input.bmp";
	cout << " outputSerialPrewitt.bmp";
	cout << " outputParallelPrewitt.bmp";
	cout << " outputSerialEdge.bmp";
	cout << " outputParallelEdge.bmp" << endl << endl;
}

int main(int argc, char* argv[])
{

	generateFilter(FILTER_SIZE, filterHor, filterVer);

	if (argc != __ARG_NUM__)
	{
		usage();
		return 0;
	}

	std::cout << "Enter the name of the image  (color.bmp, bridge.bmp, cave.bmp): ";
	std::string imageName;
	std::cin >> imageName;
	argv[1] = new char[imageName.size() + 1];
	std::copy(imageName.begin(), imageName.end(), argv[1]);
	argv[1][imageName.size()] = '\0';

	const char* validImageNames[] = { "color.bmp", "bridge.bmp", "cave.bmp"};
	bool isValidImageName = false;
	for (int i = 0; i < sizeof(validImageNames) / sizeof(validImageNames[0]); i++)
	{
		if (std::strcmp(argv[1], validImageNames[i]) == 0)
		{
			isValidImageName = true;
			break;
		}
	}

	if (!isValidImageName)
	{
		std::cout << "Cannot open the input file. Please enter a valid image name (color.bmp, bridge.bmp, cave.bmp)." << std::endl;
		return 0;
	}

	BitmapRawConverter inputFile(argv[1]);
	BitmapRawConverter outputFileSerialPrewitt(argv[1]);
	BitmapRawConverter outputFileParallelPrewitt(argv[1]);
	BitmapRawConverter outputFileSerialEdge(argv[1]);
	BitmapRawConverter outputFileParallelEdge(argv[1]);

	unsigned int width, height;

	int test;

	width = inputFile.getWidth();
	height = inputFile.getHeight();
	grain_size = width / 8;

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* midBuffer = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, argv[2], outBufferSerialPrewitt, midBuffer, width, height);

	// parallel version Prewitt
	run_test_nr(2, &outputFileParallelPrewitt, argv[3], outBufferParallelPrewitt, midBuffer, width, height);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, argv[4], outBufferSerialEdge, midBuffer, width, height);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, argv[5], outBufferParallelEdge, midBuffer, width, height);

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if (test != 0)
	{
		cout << "Edge detection FAIL!" << endl;
	}
	else
	{
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;

	delete outBufferSerialEdge;
	delete midBuffer;
	delete outBufferParallelEdge;

	return 0;
}