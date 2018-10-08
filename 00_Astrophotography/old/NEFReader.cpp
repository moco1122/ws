/*
 * NEFReader.cpp
 *
 *  Created on: 2015/10/09
 *      Author: kawai
 */


#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "NEFReader.hpp"

#include "dcraw_functions.h"

using namespace std;
using namespace cv;

NEFReader::NEFReader() {

}
NEFReader::NEFReader(string fileName) {
	this->fileName = fileName;
	cout << fileName << endl;

	nef = readNEF(fileName.c_str());
}


NEFReader::~NEFReader() {

}

