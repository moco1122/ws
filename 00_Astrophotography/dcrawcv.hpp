/*
 * dcrawcv.hpp
 *
 *  Created on: 2016/10/15
 *      Author: kawai
 */

#ifndef DCRAWCV_HPP_
#define DCRAWCV_HPP_

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

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
	cv::Mat_<ushort> I0;
	cv::Mat_<ushort> I1;

	NEF() {
		fileName = "unknown";
		date = "nodate";
		cameraName = "unknown";
		ISO = 0;
		exposure = "0";
		aperture = 0;
		focalLength=0;
		status = 0;
	};
	~NEF() {};

	void printInfoHeader() {
		cout << cv::format("%s %s %s %s %s %s %s %s","File","Date","Time","Camera",
				"ISO","Exposure","Aperture","f") << endl;
	}
	void printInfo() {
		cout << cv::format("%s %s %8s %8d %8s %4.1f %6.1f",
				fileName.c_str(),date.c_str(),cameraName.c_str(),
				ISO,exposure.c_str(),aperture,focalLength) << endl;
	}

} ;

NEF readNEF (int argc, const char **argv);

} //end of dcraw



#endif /* DCRAWCV_HPP_ */
