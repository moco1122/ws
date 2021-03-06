/*
 * listnef.cpp
 */

#include <iostream>
#include <fstream>   // ifstream, ofstream
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#include <chrono>
std::chrono::system_clock::time_point start;
inline void start_time() {
	start = std::chrono::system_clock::now();
}
inline double elapsed_time() {
	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
	return elapsed;
}

#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.hpp"
#include "Utils++.cpp"

using std::string;
using std::cout;
using std::endl;
using namespace cv;
using namespace mycv;

DEFINE_string(i0, "", "Input NEF file.");
DEFINE_string(i, "", "Input directory");
DEFINE_bool(s, false, "Print statistics mode");
DEFINE_string(E, "", "Eval area x,y,w,h");
DEFINE_string(Es, "", "Eval areas x1,y1,w,h[,wstep,hstep]");
DEFINE_string(dump, "", "Dump output");
DEFINE_int32(offset, 600, "Offset value");

//天体写真用の画像処理を行うクラス
//複数画像処理で共通するデータを内部に保持
//オプションと合わせて分離予定
class Astrophoto {

public:
	Astrophoto(int argc, char **argv);

	void processFlat();

};


int main(int argc, char **argv) {
	//オプション解析
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	//NEFを読み込み
	vector<string> inputs;
	if(!FLAGS_i0.empty()) { inputs.push_back(FLAGS_i0); }
	else if(!FLAGS_i.empty()) {
		inputs = getNEFFilesList(FLAGS_i);
	}
	if(inputs.size() <= 0) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		exit(0);
	}

	std::ofstream csv;
	if(!FLAGS_dump.empty()) {
		csv.open(FLAGS_dump, std::ios::out);
		//csv << cv::format("%48s %6s %12s %8s %8s %8s %8s %8s %8s",
		csv << cv::format("%s,%s,%s,%s,%s,%s,%s,%s,%s",
				"File", "ISO", "Exposure", "X", "Y", "R", "Gr", "Gb", "B") << endl;
	}

	//	EXIFリスト表示
	if(FLAGS_s) {
		cout << dcraw::NEF::getStatHeader() << endl;
	}
	else {
		cout << dcraw::NEF::getEXIFHeader() << endl;
	}



	start_time();
	for(std::vector<string>::iterator itr = inputs.begin(); itr != inputs.end(); ++itr) {
		string input = *itr;
		dcraw::NEF nef = dcraw::readNEF0(input);
		if(FLAGS_s) {
			if(!FLAGS_Es.empty()) {
				int x1, y1, w, h, xstep, ystep;
				vector< int > eval_args = parseCoefficients<int>(FLAGS_Es);
				x1 = eval_args[0];
				y1 = eval_args[1];
				w = eval_args[2];
				h = eval_args[3];
				if(eval_args.size() == 4) { xstep = w; ystep = h; }
				else {
					xstep = eval_args[4];
					ystep = eval_args[5];
				}
				for(int y = y1; y < (int)nef.height - h; y = y + ystep) {
					for(int x = x1; x < (int)nef.width - w; x = x + xstep) {
						//cout << x << " " << y << " " << w << " " << h << endl;
						cout << nef.getStatInfo(x + w / 2, y + h / 2, w, h) << endl;
					}
				}
			}
			else {
				int x, y, w, h;
				x = y = w = h = 0;
				if(!FLAGS_E.empty()) {
					vector< int > eval_args = parseCoefficients<int>(FLAGS_E);
					x = eval_args[0];
					y = eval_args[1];
					w = eval_args[2];
					h = eval_args[3];
				}
				cout << nef.getStatInfo(x + w / 2, y + h / 2, w, h) << endl;
			}
		}
		else {
			cout << nef.getEXIFInfo() << endl;
		}

		if(!FLAGS_dump.empty()) {
			int xc, yc, w, h;
			if(!FLAGS_E.empty()) {
				vector< int > eval_args = parseCoefficients<int>(FLAGS_E);
				xc = eval_args[0];
				yc = eval_args[1];
				w = eval_args[2];
				h = eval_args[3];
			}
			else {
				cerr << ERROR_LINE << "error" << endl; exit(0);
			}
			Mat_<short> b = nef.bayer;
			b = b - FLAGS_offset;
			for(int y = yc - h / 2; y < yc + h / 2; y = y + 2) {
				for(int x = xc - w / 2; x < xc + w / 2; x = x + 2) {
//					csv << cv::format("%48s %6d %12s %4d %4d  %8d %8d %8d %8d",
					csv << cv::format("%s,%d,%s,%d,%d,%d,%d,%d,%d",
							nef.fileName.c_str(), nef.ISO, nef.exposure.c_str(),
							x, y, b(y, x), b(y, x+1), b(y+1, x), b(y+1, x+1)) << endl;
				}
			}
		}

		//	nef.freeData();
	}
	cout << "#" << elapsed_time() << "s" << endl;
	csv.close();

	return 0;
}




