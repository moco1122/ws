/*
 * benchmarknef.cpp
 *
 *  Created on: Dec 12, 2018
 *      Author: kawai
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.hpp"
#include "MatUtils.hpp"
#include "QuickLook.hpp"
//
//using std::string;
//using std::cout;
//using std::endl;/using namespace std;
using namespace std;
using namespace cv;
using namespace mycv;

#include "Astro.hpp"

DEFINE_string(i, (std::string)"last.nef", "Input NEF file.");
DEFINE_int32(type, 0, "0:Read only / 1:Add / 2:Clone and add / 3:meanstddev");

int main(int argc, char **argv) {
	//オプション解析
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	//NEFを読み込み
	vector<string> inputs;
	inputs = getImageFilesList(FLAGS_i);
	dcraw::NEF nef;
	cout << nef.getEXIFHeader() << endl;
	start_time();
	Mat_<unsigned short> sum_bayer;
	for(auto input : inputs) {
		dcraw::NEF nef = dcraw::readNEF0(input); //36M,SSDからで0.90s
		if(FLAGS_type == 1) {
			//-iで指定したNEFを読んで現像
			if(sum_bayer.cols == 0) { sum_bayer = nef.bayer.clone(); }
			else {
				sum_bayer = sum_bayer + nef.bayer;
			}

		}
		cout << nef.getEXIFInfo() << getImageInfo(nef.bayer) << " " << elapsed_time() << endl;
	}

	return 0;
}




