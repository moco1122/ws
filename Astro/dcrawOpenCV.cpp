/*
 * dcrawOpenCV.hpp
 *
 *  Created on: 2016/10/15
 *      Author: kawai
 *
 *  NEFクラスのデータをcv:Matに変換する関数
 *  dcraw本体部分のコンパイル時の警告メッセージが多いのでdcrawFunc.cppと分けている。
 */

#include <opencv2/opencv.hpp>

#include "dcrawFunc.hpp"

using namespace std;
using namespace cv;

namespace dcraw {
//TBD data0を経由していた時代の名残
//Mat1w NEF_to_bayer(dcraw::NEF &nef) {
//	//そのままbayer配列として読み出す
//	int width = nef.width;
//	int height = nef.height;
//
//	Mat1w bayer(height, width);
//
//	for(int y = 0; y < height; y++) {
//		for(int x = 0; x < width; x++) {
//			bayer(y, x) = nef.data0[x + y*width];  //Rのarrayと縦横が逆
//		}
//	}
//
//	return(bayer);
//}
Mat1w NEF_to_rggb(dcraw::NEF &nef) {
	//bayerをRGGBに並び替えて読み出す
	int width = nef.width;
	int height = nef.height;
	int w2 = width / 2;
	int h2 = height / 2;

	Mat1w rggb(height, width);

	//unsigned int xy;
	int tx, ty;
	for(int y = 0; y < height; y += 2) {
		for(int x = 0; x < width; x += 2) {
			//xy = x + y*width;
			tx = x / 2;
			ty = y / 2;
//			rggb(ty     , tx     ) = nef.data0[xy];
//			rggb(ty     , tx + w2) = nef.data0[xy + 1];
//			rggb(ty + h2, tx     ) = nef.data0[xy + width];
//			rggb(ty + h2, tx + w2) = nef.data0[xy + 1 + width];
			rggb(ty     , tx     ) = nef.bayer(y    , x    );
			rggb(ty     , tx + w2) = nef.bayer(y    , x + 1);
			rggb(ty + h2, tx     ) = nef.bayer(y + 1, x    );
			rggb(ty + h2, tx + w2) = nef.bayer(y + 1, x + 1);
		}
	}

	return(rggb);
}

Mat3d NEF_to_srgb(dcraw::NEF &nef, unsigned short offset, double r_gain, double b_gain) {
	//1/2x1/2の簡易的なRGB画像として読み出す
	//TODO 重心一致補間にした方が良い
	//int width0 = nef.width;
	int width = nef.width/2;
	int height = nef.height/2;

	Mat3d srgb(height, width);
	double r, g, b;
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
//			r = ((double)nef.data0[(2*x  ) + (2*y  )*width0] - offset) * r_gain;
//			g = ((double)nef.data0[(2*x+1) + (2*y  )*width0] - offset);
//			b = ((double)nef.data0[(2*x+1) + (2*y+1)*width0] - offset) * b_gain;
			r = ((double)nef.bayer(2 * y    , 2 * x    ) - offset) * r_gain;
			g = ((double)nef.bayer(2 * y    , 2 * x + 1) - offset);
			b = ((double)nef.bayer(2 * y + 1, 2 * x + 1) - offset) * b_gain;
			srgb(y, x) = Vec3d(b, g, r);
		}
	}
	return(srgb);
}

Mat1w readNEFAsBayer(string fileName) {
	dcraw::NEF nef = dcraw::readNEF0(fileName);
//	Mat1w bayer = NEF_to_bayer(nef);
//	free(nef.data0);
	return(nef.bayer);
}
Mat1w readNEFAsRGGB(string fileName) {
	dcraw::NEF nef = dcraw::readNEF0(fileName);
	Mat1w rggb = NEF_to_rggb(nef);
//	free(nef.data0);
	return(rggb);
}
Mat3d readNEFAsSRGB(string fileName,
		unsigned short offset, double r_gain, double b_gain) {
	//cout << __func__ << " Not tested yet." << endl; exit(0);
	dcraw::NEF nef = dcraw::readNEF0(fileName);
	Mat3d srgb = NEF_to_srgb(nef, offset, r_gain, b_gain);
//	free(nef.data0);
	return(srgb);
}

} //namespace dcraw
