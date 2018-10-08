/*
 * Astro.hpp
 *
 *  Created on: 2018/09/17
 *
 */

#ifndef ASTRO_HPP_
#define ASTRO_HPP_
#include <iostream>
#include <string>

#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.hpp"
#include "MatUtils.hpp"
#include "QuickLook.hpp"

using std::string;
using std::cout;
using std::endl;

const double flat_value = 10000.0;

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

//Astroのパラメータ
DEFINE_int32(offset, 600, "Offset Default=600(D810A)");
DEFINE_double(gain, 1.0, "Gain");
DEFINE_string(wb, "1.0,1.0", "WB rGain,bGain");
DEFINE_bool(awb, true, "Auto White Balance");


template< typename T > Mat3b hist(Mat_< T > &I0);

//天体写真用の画像処理を行うクラス
//複数画像処理で共通するデータを内部に保持
class Astrophoto {
	//コマンドラインオプションから読み込むデフォルト値
	int offset;
	double gain;

public:
	double rGain;
	double bGain;

	Mat flat;

public:
	Astrophoto();
	~Astrophoto() {};

	template < typename T >
	Mat_< Vec< T, 3 > > bayerToRGB(Mat_<unsigned short> bayer);
	template < typename T >
	void AWB(Mat_< Vec< T, 3 > > &I0, Rect rect);
	template < typename T >
	void whiteBalance(Mat_< Vec< T, 3 > > &I0, double _rGain, double _bGain);
	template < typename T >
	void flatFielding(Mat_< Vec< T, 3 > > &I0, Mat_< T > &flat);
	template < typename T >
	void normalize(Mat_< Vec< T, 3 > > &I0, double min0, double max0, double gamma);

};

Astrophoto::Astrophoto() {
	offset = FLAGS_offset;
	gain = FLAGS_gain;
	vector<double> wbs = parseCoefficients<double>(FLAGS_wb);
	rGain = wbs[0];
	bGain = wbs[1];

}

template < typename T>
Mat_< Vec< T, 3 > > Astrophoto::bayerToRGB(Mat_<unsigned short> bayer) {
	cout << "#" << __func__ << endl;
	Mat_<T> R(bayer.rows/2, bayer.cols/2);
	Mat_<T> G(bayer.rows/2, bayer.cols/2);
	Mat_<T> B(bayer.rows/2, bayer.cols/2);
	vector< Mat_<T> > vBGR;

	//D810A
	// RG
	// GB
	int tx, ty;
	for(int y = 1; y < R.rows-1; y++) {
		for(int x = 1; x < R.cols-1; x++) {
			tx = 2 * x;
			ty = 2 * y;
			R(y, x) = (9*bayer(ty  ,tx  ) + 3*bayer(ty  ,tx+2) + 3*bayer(ty+2,tx  ) + bayer(ty+2,tx+2))/(T)16;
			G(y, x) = (9*bayer(ty  ,tx+1) + 3*bayer(ty  ,tx-1) + 3*bayer(ty+2,tx+1) + bayer(ty+2,tx-1) +
					9*bayer(ty+1,tx  ) + 3*bayer(ty+1,tx+2) + 3*bayer(ty-1,tx  ) + bayer(ty-1,tx+2))/(T)32;
			B(y, x) = (9*bayer(ty+1,tx+1) + 3*bayer(ty+1,tx-1) + 3*bayer(ty-1,tx+1) + bayer(ty-1,tx-1))/(T)16;
		}
	}

	R = R - offset;
	G = G - offset;
	B = B - offset;

	if(rGain != 1.0) { R = R * rGain; }
	if(bGain != 1.0) { B = B * bGain; }

	vBGR.push_back(B);
	vBGR.push_back(G);
	vBGR.push_back(R);

	Mat_< Vec< T, 3 > > bgr;
	cv::merge(vBGR, bgr);

	return(bgr);
}

template < typename T >
void Astrophoto::AWB(Mat_< Vec< T, 3 > > &I0, Rect rect) {
	Scalar mean, stddev;
	cv::meanStdDev(I0(rect), mean, stddev);
	cout << ERROR_LINE << " " << mean << " " << stddev << endl;
	double _rGain = mean.val[1] / mean.val[2];
	double _bGain = mean.val[1] / mean.val[0];

	whiteBalance(I0, _rGain, _bGain);
	return;
}

template < typename T >
void Astrophoto::whiteBalance(Mat_< Vec< T, 3 > > &I0, double _rGain, double _bGain) {
	cout << "#" << __func__ << " " << _rGain << " " << _bGain << endl;
	vector< Mat > vI0;
	split(I0, vI0);
	vI0[2] = vI0[2] * _rGain;
	//	vI0[1] = vI0[1] * gain;
	vI0[0] = vI0[0] * _bGain;
	rGain = rGain * _rGain;
	bGain = bGain * _bGain;
	//	Mat_< Vec< T, 3 > > I1;
	merge(vI0, I0);

	return;
}

template < typename T >
void Astrophoto::flatFielding(Mat_< Vec< T, 3 > > &I0, Mat_< T > &flat) {

	return;
}

template < typename T >
void Astrophoto::normalize(Mat_< Vec< T, 3 > > &I0, double min0, double max0, double gamma) {
	Mat_< Vec< T, 3 > > nI0 = (I0 - min0) / (max0 - min0);
	if(gamma != 1.0) {
		nI0 = cv::max(nI0, 0.0);
		pow(nI0, 1.0 / gamma, nI0);
	}
	I0 = nI0 * 255.0;

	return ;
}



template< typename T >
Mat3b hist(Mat_< T > &I0) {
	if(I0.channels() != 3) {
		cout << __LINE__ << ":" << __func__ << " I0.channels() != 3" << endl;
		exit(0);
	}
	// ヒストグラムを描画する画像割り当て
	const int ch_width = 1024, ch_height = 200;
	Mat3b H;
	vector< Mat1b > vH(3);
	vector< Mat > hists(3);
	const int hdims[] = {1024}; // 次元毎のヒストグラムサイズ
	const float hranges[] = {0, (1<<14)-1};
	const float* ranges[] = {hranges}; // 次元毎のビンの下限上限
	vector<double> max_vals(3);

	vector<Mat> vI0;
	split(I0, vI0);
	for(int b = 0; b <= 2; b++) {
		cv::calcHist(&vI0[b], 1, 0, cv::Mat(), hists[b], 1, hdims, ranges);
		max_vals[b] = 0.0;
		cv::minMaxLoc(hists[b], 0, &max_vals[b]);
	}
	//	cout << hists[0];
	double max_val = (max_vals[0] < max_vals[1]) ? max_vals[1] : max_vals[0];
	max_val = (max_val < max_vals[2]) ? max_vals[2] : max_val;
	for(int b = 0; b <= 2; b++) {
		hists[b] = hists[b] * (max_val ? ch_height / max_val : 0.0);
		vH[b] = Mat_<unsigned char>(ch_height, ch_width);
		vH[b] = 64;
		cv::rectangle(vH[b], cv::Point(ch_width * 0.25, vH[b].rows), cv::Point(ch_width * 0.25, 0), 255, -1);
		cv::rectangle(vH[b], cv::Point(ch_width * 0.50, vH[b].rows), cv::Point(ch_width * 0.50, 0), 255, -1);
		cv::rectangle(vH[b], cv::Point(ch_width * 0.75, vH[b].rows), cv::Point(ch_width * 0.75, 0), 255, -1);

		for(int j = 0; j < hdims[0]; ++j) {
			int bin_w = cv::saturate_cast<int>((double)ch_width / hdims[0]);
			cv::rectangle(vH[b],
					cv::Point(j * bin_w, vH[b].rows),
					cv::Point((j + 1) * bin_w, vH[b].rows - cv::saturate_cast<int>(hists[b].at<float>(j))),
					255, -1);
		}
	}
	merge(vH, H);

	return H;
}

#endif /* ASTRO_HPP_ */
