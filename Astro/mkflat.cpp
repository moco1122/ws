/*
 * mkflat.cpp
 *
 *  フラットフレームを簡易現像して、天体確認用の簡易現像で使えるようにtifで保存
 *  オプションでフラットの明るさ確認・統計情報・プロファイル確認も行う
 *
 *  mkflat -i NEF/20161103-035653\)-0022.nef -sigma 5 -Ds 3 -D -4 -awb //画像確認
 *  mkflat -i NEF/20161103-035653\)-0022.nef -sigma 5 -Ds 3 -H 1 -awb //ヒストグラム確認
 *  mkflat -i NEF/20161103-035653\)-0022.nef -sigma 5 -awb //flat.tifに保存
 *  mkflat -i NEF/20161103-035653\)-0022.nef //flat.tifに保存
 *
 */
#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.hpp"
#include "MatUtils.hpp"
#include "QuickLook.hpp"

using namespace std;
using std::string;
using std::cout;
using std::endl;

using namespace cv;
using namespace mycv;

#include "Astro.hpp"

DEFINE_string(i, "last.nef", "Input NEF file.");
DEFINE_string(o, "flat.tif", "Output tiff file.");
DEFINE_int32(l, (int)0, "Last lth file for default input");

DEFINE_double(sigma, 5.0, "Smoothing sigma");
//フィルタ無しだと5.0くらいまでは問題無し
//6を越えると等方的な平滑化では段差が見える
DEFINE_int32(D, 0, "Display scale");
DEFINE_double(Ds, 0.0, "Display sigma");
DEFINE_int32(H, 0, "Display histogram");

template < typename T >
class FlatMaker {
	string input;
	string output;

	double smooth_sigma;
	Astrophoto A;

	dcraw::NEF nef;
	Mat_< Vec< T, 3 > > F0; //WB補正済みの元フラット画像
	Mat_< Vec< T, 1 > > flat; //平滑化したフラット画像
	Mat_< Vec< T, 3 > > F1; //フラット補正後のフラット画像

	Range xrange;
	Range yrange;

public:
	FlatMaker(string _input, string _output);
	~FlatMaker() {};

	void check();
};

template < typename T >
FlatMaker< T >::FlatMaker(string _input, string _output) {
	//簡易現像
	//tifを出力
	input = _input;
	output = _output;
	smooth_sigma = FLAGS_sigma;

	if(input == "last.nef") {
		input = getLatestNEFFile("./", FLAGS_l);
	}
	else {
		cout << "#input : " << input << endl;
	}

	//36M,SSDからで0.90s
	nef = dcraw::readNEF0(input);
	cout << nef.getEXIFHeader() << endl;
	cout << nef.getEXIFInfo() << getImageInfo(nef.bayer) << endl;

	//floatだと0.16s
	start_time();
	F0 = A.bayerToRGB< T >(nef.bayer);
	cout << elapsed_time() << "s " << A.rGain << " " << A.bGain << endl;

	xrange = Range(F0.cols / 2 - F0.cols / 8, F0.cols / 2 + F0.cols / 8);
	yrange = Range(F0.rows / 2 - F0.rows / 8, F0.rows / 2 + F0.rows / 8);

	if(FLAGS_awb) {
		start_time();
		A.AWB(F0, Rect(xrange.start, yrange.start, xrange.size(), yrange.size()));
		cout << elapsed_time() << "s " << A.rGain << " " << A.bGain << endl;
	}

	//cout << elapsed_time() << "s" << endl;
	vector< Mat_< Vec< T, 1 > > > vF0;
	vector< Mat_< Vec< T, 1 > > > vF1;

	split(F0, vF0);
	vF1.resize(vF0.size());

	Scalar mean0, stddev0;
	Scalar mean_flat, stddev_flat;
	Scalar mean1, stddev1;
	//cv::meanStdDev(F0, mean0(yrange, xrange), stddev0);
	//cout << mean0 << " " << stddev0 << endl;

	GaussianBlur(vF0[1], flat, Size(), FLAGS_sigma);
	cv::meanStdDev(flat(yrange, xrange), mean_flat, stddev_flat);
	cout << mean_flat << " " << stddev_flat << endl;
	flat = flat / mean_flat.val[0];

	for(int b = 0; b <= 2; b++) {
		cv::divide(vF0[b], flat, vF1[b]);
	}
	merge(vF1, F1);
	cv::meanStdDev(F1(yrange, xrange), mean1, stddev1);
	cout << mean1 << " " << stddev1 << endl;
	cv::meanStdDev(F1, mean1, stddev1);
	cout << mean1 << " " << stddev1 << endl;

	cout << "#Save to " << output << endl;
	Mat1w oF1 = Mat1w(flat * flat_value);
	cout << getImageInfo(oF1) << endl;
	cv::imwrite(output.c_str(), oF1);

	return;
}

template < typename T >
void FlatMaker< T >::check() {
	//プロファイル表示

	if(FLAGS_D != 0) {
		Scalar mean0, stddev0;
		Scalar mean1, stddev1;
		cv::meanStdDev(F0(yrange, xrange), mean0, stddev0);
		cv::meanStdDev(F1(yrange, xrange), mean1, stddev1);

		double display_sigma = FLAGS_Ds;
		double min0 = mean0.val[1] - display_sigma * stddev0.val[1];
		double max0 = mean0.val[1] + display_sigma * stddev0.val[1];
		Mat dF0 = (F0 - min0) / (max0 - min0) * 255.0;

		double min1 = mean1.val[1] - display_sigma * stddev1.val[1];
		double max1 = mean1.val[1] + display_sigma * stddev1.val[1];
		Mat dF1 = (F1 - min1) / (max1 - min1) * 255.0;

		QuickLook ql;
		ql.addS(dF0, FLAGS_D, "Original");
		ql.addS(dF1, FLAGS_D, "Flat-fiedlded");
		//ql.addH("Histogram", H1);
		ql.wait();
	}
	if(FLAGS_H != 0) {
		Mat3f fF0 = F0;
		Mat3f fF1 = F1;
		Mat3b H0 = hist(fF0);
		Mat3b H1 = hist(fF1);

		QuickLook ql;
		ql.addS(H0, FLAGS_H, "Original");
		ql.addS(H1, FLAGS_H, "Flat-fielded");
		//ql.addH("Histogram", H1);
		ql.wait();

	}

	return;
}


int main(int argc, char **argv) {
	//オプション解析
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	//NEFを読み込み
	string input = FLAGS_i;
	string output = FLAGS_o;

	FlatMaker<float> flat_maker(input, output);
	flat_maker.check();

	return 0;
}



