/*
 * listnef.cpp
 */

#include <iostream>
#include <string>
#include <sys/types.h>
#include <dirent.h>

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

DEFINE_string(i, "unknown.nef", "Input NEF file.");
DEFINE_bool(s, false, "Print statistics mode");
DEFINE_string(E, "", "Eval area x,y[,wx,wy]");

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
	inputs = getImageFilesList(FLAGS_i);

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
			int x, y, w, h;
			x = y = w = h = 0;
			cout << nef.getStatInfo(x, y, w, h) << endl;
		}
		else {
			cout << nef.getEXIFInfo() << endl;
		}

		//	nef.freeData();
	}
	cout << elapsed_time() << "s" << endl;

	return 0;
}




