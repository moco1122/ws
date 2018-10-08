/*
 * dcrawcv.hpp
 *
 *  Created on: 2016/10/15
 *      Author: kawai
 */

#ifndef DCRAWFUNC_HPP_
#define DCRAWFUNC_HPP_
//OpenCVとRcppで共通する部分の関数
#include <iostream>
#include <string>
#include <stdio.h>

using namespace std;

namespace dcraw {

class NEF {

public:
	string fileName;
	string date; //class?
	string cameraName;
	int ISO;
	string exposure;//int?
	float aperture;
	float focalLength;

	int status; //dcrawの戻り値
	unsigned short *data0;
	unsigned int width,height;

	NEF() {
		fileName = "unknown";
		date = "nodate";
		cameraName = "unknown";
		ISO = 0;
		exposure = "0";
		aperture = 0;
		focalLength=0;
		status = 0;

		data0 = 0;
		width = 0;
		height = 0;
	};
	~NEF() {};

	void printInfoHeader() {
//		cout << cv::format("%s %s %s %s %s %s %s %s","File","Date","Time","Camera",
//				"ISO","Exposure","Aperture","f") << endl;
		printf("%s %s %s %s %s %s %s %s\n","File","Date","Time","Camera",
				"ISO","Exposure","Aperture","f");
	}
	void printInfo() {
//		cout << cv::format("%s %s %8s %8d %8s %4.1f %6.1f",
//				fileName.c_str(),date.c_str(),cameraName.c_str(),
//				ISO,exposure.c_str(),aperture,focalLength) << endl;
		printf("%s %s %8s %8d %8s %4.1f %6.1f\n",
				fileName.c_str(),date.c_str(),cameraName.c_str(),
				ISO,exposure.c_str(),aperture,focalLength);
	}

} ;

NEF readNEF00 (int argc, const char **argv); //dcrawのコマンドライン引数を入力して処理
NEF readNEF0 (string fileName); //readNEF00()のWrapper
//OpenCV,Rcppそれぞれで TYPE readNEF(string fileNmae)を実装

} //end of dcraw

#endif /* DCRAWCUND_HPP_ */
