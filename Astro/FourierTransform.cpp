/*
 * FourierTransform.cpp
 *
 *  Created on: 2014/05/13
 *              2018/07/12 : reject boost dependency
 *      Author: Atsushi Kawai
 *
 * OpenCVを使ったFFTの関数
 */

#include <vector>
#include <opencv2/opencv.hpp>

#include "Utils++.hpp"
#include "FourierTransform.hpp"

using namespace cv;
using namespace std;

namespace mycv {
Mat DFT(Mat I,int flag,bool square) {
	//1面実数画像2次元DFTを行い複素数画像(2面)を返す。
	//opencv-2.4.7/sample/cpp/dft.cppを修正して使用
	//square==trueの場合、周波数空間でのフィルタリングのために縦横長さをそろえる。
	if(I.channels() != 1) {
		cerr << ERROR_LINE << "I.channels() != 1. Use DFTs()." << endl; exit(-1);
	}
	if(I.depth() != CV_32F && I.depth() != CV_64F) {
		cerr << ERROR_LINE << "I.depth() != CV_32F/CV_64F. Check input type." << endl; exit(-1);
	}

	//DFTに必要なサイズに拡大
	int M = I.rows;
	if(M > 1) { M = getOptimalDFTSize( I.rows ); }
	int N = I.cols;
	if(N > 1) { N = getOptimalDFTSize( I.cols ); }

	if(square == true) {
		if(M > N) { N = M; }
		else if (M < N) { M = N; }
	}
	if(M != I.rows || N != I.cols) {
		cout << ERROR_LINE << "M != I.rows || N != I.cols ";
		cout << M << " " << I.rows << " " << N << " " << I.cols << endl;
	}
	//ここからmakefileを編集
	Mat padded;
	copyMakeBorder(I, padded, 0, M - I.rows, 0, N - I.cols, BORDER_CONSTANT, Scalar::all(0));

	//入力画像を複素行列の実数部に設定
	Mat complexImage;
	vector<Mat> planes;
	//	planes.push_back(Mat_<float>(padded));
	planes.push_back(padded);
	planes.push_back(Mat::zeros(padded.size(), padded.type()));
	merge(planes, complexImage);
	int nonzeroRows = 0;
	if((flag & DFT_ROWS) == DFT_ROWS) { nonzeroRows = I.rows; }

	//DFT
	dft(complexImage, complexImage,flag,nonzeroRows);
	return(complexImage);
}
Mat inverseDFT(Mat F,int flag) {
	//複素数画像(2面)に逆DFTを行い実数画像(1面)を返す。
	if(F.channels() != 2) { cerr << ERROR_LINE << "F.channels() != 2" << endl; exit(-1); }

	Mat I = Mat::zeros(F.rows, F.cols, F.type());
	idft(F, I,flag);
	vector<Mat> Is;
	split(I, Is);

	return(Is[0]);
}
vector<Mat> DFTs(Mat I,int flag,bool square) {
	vector<Mat> Is;
	vector<Mat> Cs;
	split(I,Is);
	Cs.resize(Is.size());
	for(unsigned int c = 0; c < Is.size(); c++) {
		Cs[c] = DFT(Is[c],flag,square);
	}
	return(Cs);
}
Mat inverseDFTs(vector<Mat> Fs,int flag) {
	vector<Mat> Is;
	Is.resize(Fs.size());
	for(unsigned int c = 0; c < Fs.size(); c++) {
		Is[c] = inverseDFT(Fs[c],flag);
	}
	Mat I;
	merge(Is,I);
	return(I);
}

Mat complexToMag(Mat F) {
	//複素数画像(2面)から振幅画像を計算して返す。
	if(F.channels() != 2) { cerr << ERROR_LINE << "C.channels() != 2" << endl; exit(-1); }

	vector<Mat> Fs;
	split(F,Fs);
	//メモリ開放は？
	//	Mat planes[] = {Mat::zeros(F.size(), CV_32F), Mat::zeros(F.size(), CV_32F)};
	//	split(F, planes);
	magnitude(Fs[0],Fs[1],Fs[0]);
	Mat A = Fs[0];
	return(A);
}

void swapQuadrant(Mat F) {
	// rearrange the quadrants of Fourier image
	// so that the origin is at the image center

	if((F.cols%2 != 0) ||(F.rows%2 != 0)) { cerr << ERROR_LINE << "F.cols%2 != 0 || F.rows%2 != 0" << endl; exit(-1); }
	//mag = mag(Rect(0, 0, mag.cols & -2, mag.rows & -2)); //-2=0b11111110なので奇数の場合は1小さい偶数に変換される。
	//幅・高さを2の倍数に丸めた領域で計算。エラー終了する。

	int cx = F.cols/2;
	int cy = F.rows/2;
	Mat tmp;
	Mat q0(F, Rect(0, 0, cx, cy));
	Mat q1(F, Rect(cx, 0, cx, cy));
	Mat q2(F, Rect(0, cy, cx, cy));
	Mat q3(F, Rect(cx, cy, cx, cy));

	q0.copyTo(tmp);
	q3.copyTo(q0);
	tmp.copyTo(q3);

	q1.copyTo(tmp);
	q2.copyTo(q1);
	tmp.copyTo(q2);
}
void swapHorizontal(Mat F) {
	// rearrange the quadrants of Fourier image
	// so that the origin is at the image center
	//mag = mag(Rect(0, 0, mag.cols & -2, mag.rows & -2)); //-2=0b11111110なので奇数の場合は1小さい偶数に変換される。

	if(F.cols%2 != 0 || (F.rows != 1 && F.rows%2 != 0)) { cerr << ERROR_LINE << "F.cols%2 != 0 || F.rows%2 != 0" << endl; exit(-1); }

	int cx = F.cols/2;
	Mat tmp;
	Mat q0(F, Rect(0, 0, cx, F.rows));
	Mat q1(F, Rect(cx, 0, cx, F.rows));

	q0.copyTo(tmp);
	q1.copyTo(q0);
	tmp.copyTo(q1);
}
void swapVertical(Mat F) {
	// rearrange the quadrants of Fourier image
	// so that the origin is at the image center
	//mag = mag(Rect(0, 0, mag.cols & -2, mag.rows & -2)); //-2=0b11111110なので奇数の場合は1小さい偶数に変換される。

	if((F.cols != 1 && F.cols%2 != 0) || F.rows%2 != 0) { cerr << ERROR_LINE << "F.cols%2 != 0 || F.rows%2 != 0" << endl; exit(-1); }

	int cy = F.rows/2;
	Mat tmp;
	Mat q0(F, Rect(0, 0, F.cols, cy));
	Mat q1(F, Rect(0,cy, F.cols, cy));

	q0.copyTo(tmp);
	q1.copyTo(q0);
	tmp.copyTo(q1);
}

vector<Mat> complexToMag(vector<Mat> Fs) {
	vector<Mat> As;
	As.resize(Fs.size());
	for(unsigned int c = 0; c < Fs.size(); c++) {
		As[c] = complexToMag(Fs[c]);
	}
	return(As);
}
void swapQuadrant(vector<Mat> Fs) {
	for(unsigned int c = 0; c < Fs.size(); c++) {
		swapQuadrant(Fs[c]);
	}
}
void swapHorizontal(vector<Mat> Fs) {
	for(unsigned int c = 0; c < Fs.size(); c++) {
		swapQuadrant(Fs[c]);
	}
}

//cv::mulSpectrums()を使用する。
//Mat multiplyComplex(Mat F,Mat G) {
//	//dftで変換される2チャンネルの複素行列A,Bの積を計算する。
//	if(F.channels() != 2 || G.channels() != 2) {
//		cerr << ERROR_LINE << "F.channels() != 2 || G.channels() != 2" << endl; exit(-1);
//	}
//	if(F.size() != G.size()) {
//		cerr << ERROR_LINE << "F.size() != G.size)" << endl; exit(-1);
//	}
//
//cout << ERROR_LINE << "Not implemented yet." << endl;
//	vector<Mat> Fs;
//	vector<Mat> Gs;
//	split(F,Fs);
//	split(G,Gs);
//
//	cout << ERROR_LINE << "Not implemented yet." << endl;
//	vector<Mat> Hs;
//	Hs.push_back(Mat(F.size(),F.type()));
//	Hs.push_back(Mat(F.size(),F.type()));
//	Mat H;
//#define Re 0
//#define Im 1
//	Hs[Re] = Fs[Re].mul(Gs[Re]) - Fs[Im].mul(Gs[Im]);
//	Hs[Im] = Fs[Im].mul(Gs[Re]) + Fs[Re].mul(Gs[Im]);
//#undef Re
//#undef Im
//	cout << ERROR_LINE << "Not implemented yet." << endl;
//
//	merge(Hs,H);
//	return(H);
//}
//vector<Mat> multiplyComplex(vector<Mat> Fs,vector<Mat> Gs) {
//	if(Fs.size() != Gs.size()) {
//		cerr << ERROR_LINE << "Error : Fs.size() != Gs.size()" << endl; exit(-1);
//	}
//	vector<Mat> Hs;
//	Hs.resize(Fs.size());
//	for(unsigned int c = 0; c < Fs.size(); c++) {
//		Hs[c] = multiplyComplex(Fs[c],Gs[c]);
//	}
//	return(Hs);
//}

vector<float> squaredAverageProfile(Mat_<float> A,AverageType type) {
	//振幅絶対値の二乗平均プロファイルを出力する。
	//Aはcv::dftの後swapQuadrantでtypeに応じた並べ替えを行った振幅絶対値の分布とする。
	int kLen;
	int k = 0;
	int xc,yc;

	vector<float> avgA;
	vector<int> nA;
	if(type == RADIAL) {
		if(A.cols != A.rows || A.cols%2 != 0) {
			cerr << ERROR_LINE << "error " << A.cols << " " << A.rows << endl; exit(-1);
		}

		kLen = A.cols/2 + 1;//k=0...N/2
		avgA.resize(kLen,0.0);
		nA.resize(kLen,0);
		xc = A.cols/2;
		yc = A.rows/2;
		for(int y = 0; y < A.rows; y++) {
			for(int x = 0; x < A.cols; x++) {
				k = round(sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc)));
				if( k < kLen) {
					//					avgA[k] = avgA[k] + A(y,x);
					avgA[k] = avgA[k] + A(y,x)*A(y,x);
					nA[k] = nA[k] + 1;
				}
			}
		}


		for(int k = 0; k < kLen; k++) {
			if(nA[k] > 0) {
				//				avgA[k] = avgA[k] / nA[k];
				avgA[k] = avgA[k] / nA[k]; //パワースペクトル
			}
			else { avgA[k] = 0; }
		}

	}
	//	else if(type == RADIALHALF) {
	//	使う場合は二乗平均となるように修正すること
	//		if(A.cols != A.rows || A.cols%2 != 0) {
	//			cerr << ERROR_LINE << "error " << A.cols << " " << A.rows << endl; exit(-1);
	//		}
	//
	//		kLen = A.cols/2 + 1;//k=0...N/2
	//		avgA.resize(kLen,0.0);
	//		nA.resize(kLen,0);
	//		xc = A.cols/2;
	//		yc = A.rows/2;
	//		for(int y = 0; y < A.rows/2; y++) {
	//			for(int x = 0; x < A.cols; x++) {
	//				k = round(sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc)));
	//				if( k < kLen) {
	//					avgA[k] = avgA[k] + A(y,x);
	//					nA[k] = nA[k] + 1;
	//				}
	//			}
	//		}
	//		{
	//			int y = A.rows/2;
	//			for(int x = 0; x <= A.cols/2; x++) {
	//				k = round(sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc)));
	//				if( k < kLen) {
	//					avgA[k] = avgA[k] + A(y,x);
	//					nA[k] = nA[k] + 1;
	//				}
	//			}
	//		}
	//
	//		for(int k = 0; k < kLen; k++) {
	//			if(nA[k] > 0) {
	//				avgA[k] = avgA[k] / nA[k];
	//			}
	//			else { avgA[k] = 0; }
	//		}
	//
	//	}
	//	else if(type == Vertical) {
	//	使う場合は二乗平均となるように修正すること
	//		kLen = A.rows+1;
	//		avgA.resize(kLen,0.0);
	//		nA.resize(kLen,0);
	//		for(int y = 0; y < A.rows; y++) {
	//			for(int x = 0; x < A.cols; x++) {
	//				avgA[y] = avgA[y] + A(y,x);
	//				nA[y] = nA[y] + 1;
	//			}
	//		}
	//
	//		for(int k = 0; k < kLen; k++) {
	//			if(nA[k] > 0) {
	//				avgA[k] = avgA[k] / nA[k];
	//			}
	//			else { avgA[k] = 0; }
	//			cout << k << " " << avgA[k] << endl;
	//		}
	//	}
	else {
		cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
	}

	return(avgA);
}

vector<float> getFFT2DAmplitudeProfile(Mat_<float> I) {
	//入力実数画像のDFTから振幅プロファイルを計算して返す。
	Mat F = DFT(I);
	Mat_<float> A = complexToMag(F);
	swapQuadrant(A);
	vector<float> avgA = squaredAverageProfile(A,RADIAL);
	return(avgA);
}
float calcNoWindow(float r) {
	return(1.0);
}
float calcRectanglarWindow(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) { w = 1.0; }
	return(w);
}
float calcBartlettWindow(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) { w = 1.0 - 2.0 * abs(r - 0.5); }
	return(w);
}
float calcHannWindow(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) { w = 0.5 - 0.5 * cos(2 * M_PI * r); }
	return(w);
}
float calcHammingWindow(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) { w = 0.54 - 0.46 * cos(2 * M_PI * r); }
	return(w);
}

float calcBlackmanWindow(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) { w = 0.42 - 0.5 * cos(2 * M_PI * r) + 0.08 * cos(4 * M_PI * r); }
	return(w);
}
float calcBlackmanHarris4Window(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) {
		w = 0.35875 - 0.48829 * cos(2 * M_PI * r)
		+ 0.14128 * cos(4 * M_PI * r) - 0.01168 * cos(6 * M_PI * r);
	}
	return(w);
}
float calcBlackmanHarris7Window(float r) {
	float w = 0.0;
	if(r >= 0 && r <= 1) {
		w = 0.27105140069342 - 0.43329793923448 * cos(2 * M_PI * r)
		+ 0.21812299954311 * cos(4 * M_PI * r) - 0.06592544638803 * cos(6 * M_PI * r)
		+ 0.01081174209837 * cos(8 * M_PI * r) - 0.00077658482522 * cos(10 * M_PI * r)
		+ 0.00001388721735 * cos(12 * M_PI * r);
	}
	return(w);
}

Mat get2DWindowFunction(WindowFunctionType type,int width,int height,int channels) {
	//画像中心に回転対称な窓関数を返す。2D-Blackman-Harris窓関数(正式名？)を返す。
	Mat_<float> w(height,width);
	float r;

	int xc = w.cols/2;
	int yc = w.rows/2;
	int len = max(width,height);

	float (*windowFunction)(float);
	switch(type) {
	case NO_WINDOW : windowFunction = calcNoWindow; break;
	case RECTANGLAR : windowFunction = calcRectanglarWindow; break;
	case BARTLETT : windowFunction = calcBartlettWindow; break;
	case HANN : windowFunction = calcHannWindow; break;
	case HAMMING : windowFunction = calcHammingWindow; break;
	case BLACKMAN : windowFunction = calcBlackmanWindow; break;
	case BLACKMAN_HARRIS4 : windowFunction = calcBlackmanHarris4Window; break;
	case BLACKMAN_HARRIS7 : windowFunction = calcBlackmanHarris7Window; break;
	default :  cerr << ERROR_LINE << "Unknown type : " << type << endl; exit(-1); break;
	}

	for(int y = 0; y < w.rows; y++) {
		for(int x = 0; x < w.cols; x++) {
			r = 0.5 + sqrt((x - xc) * (x - xc) + (y - yc) * (y - yc)) / len; //画像の中央が窓関数の中央(0.5)となる
			w(y,x) = (*windowFunction)(r);
		}
	}
	Mat W;
	vector< Mat > Wv(channels,w);
	merge(Wv,W);
	return(W);

}

Mat applyWindowFunction(Mat I, Mat W, bool zeroCentering) {
	Mat O = I.mul(W); //zeroCentering==trueの場合も加重平均算出用に一旦窓関数を適用

	if(zeroCentering == true) {
		//窓関数適用後の平均値が0となるように0センタリング後、窓関数を適用
		Scalar meanW, stddevW;
		meanStdDev(W, meanW, stddevW);
		Scalar meanIw, stddevIw;
		meanStdDev(O, meanIw, stddevIw);
		//				cout << meanW << " " << mean << endl;
		for(int i = 0; i < O.channels(); i++) {
			meanIw.val[i] = meanIw.val[i] / meanW.val[i];
		}
		//				cout << mean << endl;
		Mat mI(I.rows, I.cols, I.type(), meanIw);
		O = I - mI; //元画像から0センタリング
		O = O.mul(W);
		//		meanStdDev(E,mean,stddev);
	}
	return(O);
}
//
//
//Mat getHorizontalWindowFunction(int width) {
//	//MyCV/FourierTransform.cppから、
//	//type=calcBlackmanHarris7Window,channels=1で固定となるようにして移植
//	Mat_<double> w(1,width);
//	double r;
//
//	int xc = w.cols/2;
//	int yc = 0;
//	int len = width;
//
//	double (*windowFunction)(double) = calcBlackmanHarris7Window;
//
//	int y = 0;
//	for(int x = 0; x < w.cols; x++) {
//		r = 0.5 + sqrt((x - xc) * (x - xc)+(y - yc)*(y - yc))/len; //画像の中央が窓関数の中央(0.5)となる
//		w(0,x) = (*windowFunction)(r);
//	}
//	return(w);
//}
Mat applyHorizontalWindowFunction(Mat I, Mat W, bool zeroCentering = true) {
	Mat O = I.clone();
	for(int y = 0; y < I.rows; y++) {
		Mat tI = I.row(y);
		Mat tO = tI.mul(W); //zeroCentering==trueの場合も加重平均算出用に一旦窓関数を適用

		if(zeroCentering == true) {
			//窓関数適用後の平均値が0となるように0センタリング後、窓関数を適用
			Scalar meanW,stddevW;
			meanStdDev(W,meanW,stddevW);
			Scalar meanIw,stddevIw;
			meanStdDev(tO,meanIw,stddevIw);
			//				cout << meanW << " " << mean << endl;
			for(int i = 0; i < tO.channels(); i++) {
				meanIw.val[i] = meanIw.val[i] / meanW.val[i];
			}
			//				cout << mean << endl;
			Mat mI(tI.rows, tI.cols, tI.type(), meanIw);
			tO = tI - mI; //元画像から0センタリング
			tO = tO.mul(W);
			//		meanStdDev(E,mean,stddev);
		}
		tO.copyTo(O.row(y));
	}
	return(O);
}

//
//Mat getHorizontalWindowFunction(int width) {
//	//MyCV/FourierTransform.cppから、
//	//type=calcBlackmanHarris7Window,channels=1で固定となるようにして移植
//	Mat_<double> w(1,width);
//	double r;
//
//	int xc = w.cols/2;
//	int yc = 0;
//	int len = width;
//
//	double (*windowFunction)(double) = calcBlackmanHarris7Window;
//
//	int y = 0;
//	for(int x = 0; x < w.cols; x++) {
//		r = 0.5 + sqrt((x - xc) * (x - xc)+(y - yc)*(y - yc))/len; //画像の中央が窓関数の中央(0.5)となる
//		w(0,x) = (*windowFunction)(r);
//	}
//	return(w);
//}
//Mat applyHorizontalWindowFunction(Mat I,Mat W,bool zeroCentering) {
//	Mat O = I.clone();
//	for(unsigned int y = 0; y < I.rows; y++) {
//		Mat tI = I.row(y);
//		Mat tO = tI.mul(W); //zeroCentering==trueの場合も加重平均算出用に一旦窓関数を適用
//
//		if(zeroCentering == true) {
//			//窓関数適用後の平均値が0となるように0センタリング後、窓関数を適用
//			Scalar meanW,stddevW;
//			meanStdDev(W,meanW,stddevW);
//			Scalar meanIw,stddevIw;
//			meanStdDev(tO,meanIw,stddevIw);
//			//				cout << meanW << " " << mean << endl;
//			for(int i = 0; i < tO.channels(); i++) {
//				meanIw.val[i] = meanIw.val[i] / meanW.val[i];
//			}
//			//				cout << mean << endl;
//			Mat mI(tI.rows, tI.cols, tI.type(), meanIw);
//			tO = tI - mI; //元画像から0センタリング
//			tO = tO.mul(W);
//			//		meanStdDev(E,mean,stddev);
//		}
//	}
//	return(O);
//}


Mat getDFTAmplitude(Mat I,int flag,bool square) {
	vector< Mat > Fv = DFTs(I,flag,square);
	vector< Mat > aFv = complexToMag(Fv);
	swapQuadrant(aFv);
	Mat aF;
	merge(aFv,aF);
	return(aF);
}

Mat_<float> convertToPolarImage(Mat_<float> I,int width) {
	Mat_<float> P(I.cols/2,width);
	P = 0.0;
	float r,theta;
	float fx,fy;
	int xc = I.cols/2;
	int yc = I.rows/2;
	int ix,iy;
	float w1,w2,w3,w4;
	for(int y = 0; y < P.rows; y++) {
		for(int x = 0; x < P.cols; x++) {
			r = y;
			theta = M_PI*x/P.cols;
			fx = xc + r*cos(theta);
			fy = yc + r*sin(theta);
			//			ix = round(fx);
			//			iy = round(fy);
			//			P(y,x) = A(iy,ix);
			ix = floor(fx);
			iy = floor(fy);
			w1 = (ix+1-fx)*(iy+1-fy);//(x,y)
			w2 = (fx-ix)*(iy+1-fy);//(x+1,y)
			w3 = (ix+1-fx)*(fy-iy);//(x,y+1)
			w4 = (fx-ix)*(fy-iy);//(x+1,y+1);
			P(y,x) = w1*I(iy,ix) + w2*I(iy,ix+1) + w3*I(iy+1,ix) + w4*I(iy+1,ix+1);
		}
	}

	return(P);
}
Mat convertToPolarImage(Mat I,int width) {
	vector< Mat_<float> > Iv;
	split(I,Iv);
	for(unsigned int c = 0; c < Iv.size(); c++) {
		Iv[c] = convertToPolarImage(Iv[c],width);
	}
	Mat P;
	merge(Iv,P);
	return(P);
}

Mat bandpassFiltering(Mat F,float freqMin,float freqMax,float &areaRatio,
		float freqMinSigma,float freqMaxSigma,bool flag) {
	//swapQuadrant()で周波数原点が画像中央になるように並べ替えられていること
	if(F.channels() != 2) {
		cerr << ERROR_LINE << "F.channels() != 2" << endl; exit(-1);
	}

	vector< Mat_<float> > Fv;
	split(F,Fv);

	int xc = F.cols/2;
	int yc = F.rows/2;

	float frequency;
	float u,v;
	float factor = 0.0;
	double factorSum = 0.0;
	float freqMinSigma2 = -1.0/2.0/freqMinSigma/freqMinSigma;
	float freqMaxSigma2 = -1.0/2.0/freqMaxSigma/freqMaxSigma;
	for(int y = 0; y < F.rows; ++y) {
		for(int x = 0; x < F.cols; ++x) {
			if(x == xc && y == yc) { continue; } //DC成分はそのまま
			u = (float)(x-xc)/F.cols;
			v = (float)(y-yc)/F.rows;
			frequency = sqrt(u*u + v*v);
			factor = 1.0;
			if(frequency < freqMin ) {
				if(freqMinSigma <= 0.0) { factor = 0.0; }
				else {
					factor = exp((frequency-freqMin)*(frequency-freqMin)*freqMinSigma2);
				}
			}
			else if(frequency > freqMax) {
				if(freqMaxSigma <= 0.0) { factor = 0.0; }
				else {
					factor = exp((frequency-freqMax)*(frequency-freqMax)*freqMaxSigma2);
				}
			}

			if(flag == false) { factor = 1.0 - factor; }
			Fv[0](y,x) = Fv[0](y,x)*factor;
			Fv[1](y,x) = Fv[1](y,x)*factor;
			factorSum = factorSum + factor;
		}
	}

	Mat G;
	merge(Fv,G);

	areaRatio = (float)(factorSum/(F.cols*F.rows));
	return(G);
}
Mat gaussianBandpassFiltering(Mat F,float freq,float sigma,float &areaRatio,bool flag) {
	//swapQuadrant()で周波数原点が画像中央になるように並べ替えられていること
	if(F.channels() != 2) {
		cerr << ERROR_LINE << "F.channels() != 2" << endl; exit(-1);
	}

	vector< Mat_<float> > Fv;
	split(F,Fv);

	int xc = F.cols/2;
	int yc = F.rows/2;

	float frequency;
	float u,v;
	float factor = 0.0;
	double factorSum = 0.0;
	for(int y = 0; y < F.rows; ++y) {
		for(int x = 0; x < F.cols; ++x) {
			if(x == xc && y == yc) { continue; } //DC成分はそのまま
			u = (float)(x-xc)/F.cols;
			v = (float)(y-yc)/F.rows;
			frequency = sqrt(u*u + v*v);
			factor = exp(-(frequency-freq)*(frequency-freq)/2.0/sigma/sigma);
			if(flag == false) { factor = 1.0 - factor; }
			Fv[0](y,x) = Fv[0](y,x)*factor;
			Fv[1](y,x) = Fv[1](y,x)*factor;
			factorSum = factorSum + factor;
		}
	}

	Mat G;
	merge(Fv,G);

	areaRatio = (float)(factorSum / (F.cols*F.rows));
	return(G);
}

Mat logRatioGaussianBandpassFiltering(Mat F,float freq,float sigma,float &areaRatio,bool flag) {
	//swapQuadrant()で周波数原点が画像中央になるように並べ替えられていること
	if(F.channels() != 2) {
		cerr << ERROR_LINE << "F.channels() != 2" << endl; exit(-1);
	}

	vector< Mat_<float> > Fv;
	split(F,Fv);

	int xc = F.cols/2;
	int yc = F.rows/2;

	float frequency;
	float u,v;
	float factor = 0.0;
	double factorSum = 0.0;
	for(int y = 0; y < F.rows; ++y) {
		for(int x = 0; x < F.cols; ++x) {
			if(x == xc && y == yc) { continue; } //DC成分はそのまま
			u = (float)(x-xc)/F.cols;
			v = (float)(y-yc)/F.rows;
			frequency = sqrt(u*u + v*v);
			factor = exp(-sigma*pow(log(frequency/freq),2.0));
			if(flag == false) { factor = 1.0 - factor; }
			Fv[0](y,x) = Fv[0](y,x)*factor;
			Fv[1](y,x) = Fv[1](y,x)*factor;
			factorSum = factorSum + factor;
		}
	}
	Mat G;
	merge(Fv,G);

	areaRatio = (float)(factorSum / (F.cols*F.rows));
	return(G);
}
Mat createAverageImage(Mat I,AverageType type) {
	vector< Mat_<float> > Iv;
	vector< Mat_<float> > Av;

	if(I.channels() == 1) { Iv.resize(1); Iv[0] = I; }
	else { split(I,Iv); }

	Av.resize(Iv.size());
	for(unsigned int c = 0; c < Iv.size(); c++) {
		Av[c] = Mat_<float>(I.rows,I.cols);
	}

	switch(type) {
	//	case Horizontal :
	//		for(unsigned int c = 0; c < Iv.size(); c++) {
	//			for(int y = 0; y < I.rows; y++) {
	//				double sum = 0.0;
	//				for(int x = 0; x < I.cols; x++) {
	//					sum = sum + Iv[c](y,x);
	//				}
	//				sum = sum / (double)I.cols;
	//				for(int x = 0; x < I.cols; x++) {
	//					Av[c](y,x) = sum;
	//				}
	//			}
	//		}
	//		break;
	//	case Vertical :
	//		cout << ERROR_LINE << "Not implemented yet." << endl;
	//		break;
	default :
		cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
	}

	Mat A;
	merge(Av,A);
	return(A);
}

//以下でatan2(y,x)の仕様確認済
//#define aaa(x,y) cout << (x) << " " << (y) << " " << TO_DEGREE(atan2((y),(x))) << " " << sqrt(2)*cos(atan2((y),(x))) << " " << sqrt(2)*sin(atan2((y),(x)))<< endl
//aaa(1,1); aaa(-1,1); aaa(1,-1); aaa(-1,-1);

//TODO cartToPolar()/polarToCart()をつかって書き換え?
Mat convertToPolarForm(Mat O) {
	//DFTによる2面複素数画像を直交形式(z=x+iy:O[0]=x,O[1]=y)から極形式(z=rexp(itheta):P[0]=r,P[1]=theta)に変換する。
	if(O.channels() != 2) { cerr << ERROR_LINE << "O.channels() != 2" << endl; exit(-1); }

	vector< Mat_<float> > Ov;
	split(O,Ov);
	vector< Mat_<float> > Pv(2);
	Pv[0] = Mat_<float>(O.rows,O.cols,0.0);
	Pv[1] = Mat_<float>(O.rows,O.cols,0.0);

	for(int y = 0; y < O.rows; y++) {
		for(int x = 0; x < O.cols; x++) {
			Pv[0](y,x) = sqrt(Ov[0](y,x)*Ov[0](y,x) + Ov[1](y,x)*Ov[1](y,x));
			Pv[1](y,x) = atan2(Ov[1](y,x),Ov[0](y,x));
		}
	}

	Mat P;
	merge(Pv,P);
	return(P);
}
vector< Mat > convertToPolarForm(vector< Mat > Os) {
	vector< Mat > Ps(Os.size());

	for(unsigned int i = 0; i < Os.size(); i++) {
		Ps[i] = convertToPolarForm(Os[i]);
	}
	return(Ps);
}
Mat convertToOrthogonalForm(Mat P) {
	//DFTによる2面複素数画像を極形式(z=rexp(itheta):P[0]=r,P[1]=theta)から直交形式(z=x+iy:O[0]=x,O[1]=y)に変換する。
	if(P.channels() != 2) { cerr << ERROR_LINE << "P.channels() != 2" << endl; exit(-1); }

	vector< Mat_<float> > Pv;
	split(P,Pv);
	vector< Mat_<float> > Ov(2);
	Ov[0] = Mat_<float>(P.rows,P.cols,0.0);
	Ov[1] = Mat_<float>(P.rows,P.cols,0.0);

	for(int y = 0; y < P.rows; y++) {
		for(int x = 0; x < P.cols; x++) {
			Ov[0](y,x) = Pv[0](y,x)*cos(Pv[1](y,x));
			Ov[1](y,x) = Pv[0](y,x)*sin(Pv[1](y,x));
		}
	}

	Mat O;
	merge(Ov,O);
	return(O);
}
vector< Mat > convertToOrthogonalForm(vector< Mat > Ps) {
	vector< Mat > Os(Ps.size());

	for(unsigned int i = 0; i < Ps.size(); i++) {
		Os[i] = convertToOrthogonalForm(Ps[i]);
	}
	return(Os);
}

vector< Mat_<float> > createPolarFilters(int w,int h,vector<float> rCenters,float rSigma,
		int nAngles,float dSigma,float freq2cpd,bool nyquistCutoff) {
	if(w != h) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		cerr << ERROR_LINE << "error" << endl; exit(-1);
	}
	//OK// cout << freq2cpd << " " << dSigma << " " << rSigma << endl;
	//	cout << nAngles << " " << rCenters.size() << " ";
	//	for(unsigned int i = 0; i < rCenters.size(); i++) {
	//		cout << rCenters[i] << " ";
	//	}
	//	cout << endl;

	vector< Mat_<float> > vW(rCenters.size()*nAngles);

	float cpd,theta;
	int xc = w/2;
	int yc = h/2;
	int n = 0;
	for(unsigned int r = 0; r <  rCenters.size(); r = r + 1) {
		for(int d = 0; d < nAngles; d = d + 1) {
			//		for(int n = 0; n <= 8; n = n + 1) {
			vW[n] = Mat_<float>(w,h,0.0);
			//			Mat_<float> A = vW[n];
			float dCenter = 360.0/(2*nAngles)*d;

			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					cpd = sqrt((x-xc)*(x-xc) + (y-yc)*(y-yc)) / w;
					if(nyquistCutoff == true && cpd > 0.5) { continue; }
					cpd = cpd * freq2cpd;
					theta = atan2(y-yc,x-xc)*180.0/M_PI;
					theta = abs(theta - dCenter) > abs(theta-360-dCenter) ? theta-360 : theta;
					theta = abs(theta - dCenter) > abs(theta+360-dCenter) ? theta+360 : theta;
					//					if(d == 0 && r == 0) {
					//						cout << " " << x << " " << y << " " << " " << cpd << " " << theta << " " << dCenter << " " << x-xc<< " " << atan2(y - yc, x - xc)  << endl;
					//					}

					vW[n](y,x) = vW[n](y,x) + exp(-pow(log(cpd/rCenters[r]),2.0)/2.0/rSigma/rSigma) *
							exp(-pow((theta-dCenter),2.0)/2.0/dSigma/dSigma);
				}
			}
			//			Scalar mean,stddev;
			//			meanStdDev(vW[n],mean,stddev);
			//			cout << n << " " << r << " " << d << " " << mean << " " << stddev << endl;
			//			cout << getImageInfo(vW[n]) << endl;
			n++;
		}
	}

	return(vW);
}

} /* namespace mycv */
