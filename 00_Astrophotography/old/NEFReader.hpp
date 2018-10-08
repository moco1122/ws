/*
 * NEFReader.hpp
 *
 *  Created on: 2015/10/09
 *      Author: kawai
 */

#ifndef NEFREADER_HPP_
#define NEFREADER_HPP_

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "dcraw_functions.h"

using namespace std;
using namespace cv;
class NEFReader {
public:
	string fileName;
	NEF nef;
public:
	NEFReader();
	NEFReader(string fileName);
	virtual ~NEFReader();

private:

};

#endif /* NEFREADER_HPP_ */
