/*
 * dcrawFunc.hpp
 *
 *  Created on: 2016/10/15
 *      Author: kawai
 *
 *  dcrawを元にしたNEF読込クラス
 */

#ifndef DCRAWFUNC_HPP_
#define DCRAWFUNC_HPP_
//OpenCVとRcppで共通する部分の関数
//#include <iostream>
#include <string>
#include <stdio.h>

#include <opencv2/opencv.hpp>
#include "Utils++.hpp"

using std::string;
using namespace cv;
using namespace mycv;

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
	//unsigned short *data0;
	Mat_<unsigned short> bayer;
	unsigned int width,height;
	int res;

	NEF() {
		fileName = "unknown";
		date = "nodate";
		cameraName = "unknown";
		ISO = 0;
		exposure = "0";
		aperture = 0;
		focalLength=0;
		status = 0;

		//data0 = 0;
		width = 0;
		height = 0;
		res = 0;
	};
	~NEF() {
		//cout << __func__ << endl;
		//bayer.release();
//		if(data0) { free(data0); }
	};

	static string getEXIFHeader() {
		string header = cv::format("%48s %10s %8s %8s %6s %6s %8s %12s %12s %12s",
				"File", "Date", "Time", "Camera", "Width", "Height",
				"ISO", "Exposure", "Aperture", "FocalLength");
		return header;
	}
	string getEXIFInfo() {
		//readNEFに移動
		fileName = getLeafname(fileName);
		cameraName.erase(std::find(cameraName.begin(), cameraName.end(), '\"'));
		cameraName.erase(std::find(cameraName.begin(), cameraName.end(), '\"'));
		string str_exp = exposure;
		double exposure_s;
		if(str_exp.find("s", 0) < str_exp.length()) {
			str_exp.erase(std::find(str_exp.begin(), str_exp.end(), 's'));
			exposure_s = stof(str_exp);
		}
		else {
			exposure_s = 1.0 / stof(str_exp);
		}
//		cout << str_exp.find("s", 0) << str_exp.length()<< endl;
		//str_exp.erase(std::find(str_exp.begin(), str_exp.end(), 's'));

		//double exp_s =
		string info = cv::format("%48s %19s %8s %6d %6d %8d %12f %12.1f %12.1f ",
				fileName.c_str(), date.c_str(), cameraName.c_str(), width, height,
				ISO, exposure_s, aperture, focalLength);
		return info;
	}

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

} ; //class NEF

NEF readNEF00 (int argc, const char **argv); //dcrawのコマンドライン引数を入力して処理
NEF readNEF0 (string fileName); //readNEF00()のWrapper
//OpenCV,Rcppそれぞれで TYPE readNEF(string fileNmae)を実装

Mat1w NEF_to_bayer(dcraw::NEF &nef);
Mat1w NEF_to_rggb(dcraw::NEF &nef);
Mat3d NEF_to_srgb(dcraw::NEF &nef, unsigned short offset, double r_gain, double b_gain);
Mat1w readNEFAsBayer(string fileName);
Mat1w readNEFAsRGGB(string fileName);
Mat3d readNEFAsSRGB(string fileName, unsigned short offset, double r_gain, double b_gain);

} //namespace dcraw

#endif /* DCRAWFUNC_HPP_ */
