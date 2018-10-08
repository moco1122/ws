/*
 * dcrawcv.hpp
 */

#ifndef DCRAWFUNC_HPP_
#define DCRAWFUNC_HPP_
//OpenCVとRcppで共通する部分の関数
#include <iostream>
#include <string>
//#include <stdio.h>

#include <opencv2/opencv.hpp>

using std::string;
using std::cout;
using std::endl;

#include "Utils++.hpp"
using namespace mycv;
using namespace cv;

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
	unsigned int width,height;
	Mat_<unsigned short> bayer;

	NEF() {
		fileName = "unknown";
		date = "nodate";
		cameraName = "unknown";
		ISO = 0;
		exposure = "0";
		aperture = 0;
		focalLength = 0;
		status = 0;

		width = 0;
		height = 0;
	};
	~NEF() {
	};

	void freeData() { return; }

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

	//TODO deprecated
	void printInfoHeader() {
		cout << getEXIFHeader() << endl;
	}
	void printInfo() {
		cout << getEXIFInfo() << endl;
	}

} ;

NEF readNEF00 (int argc, const char **argv, bool exif_only = false); //dcrawのコマンドライン引数を入力して処理
NEF readNEF0 (string fileName, bool _exif_only = false); //readNEF00()のWrapper
//OpenCV,Rcppそれぞれで TYPE readNEF(string fileNmae)を実装

} //end of dcraw

#endif /* DCRAWCUND_HPP_ */
