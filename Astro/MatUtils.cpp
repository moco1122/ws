/*
 * MatUtils.cpp
 *
 *	OpenCVのMatを用いた画像処理のための共通関数
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <tiff.h>
#include <tiffio.h>

#include "Utils++.hpp"
#include "MatUtils.hpp"

using namespace cv;
using namespace std;

namespace mycv {

//Matの基本情報を表示。読込確認用。
string getImageInfo(Mat I) {
	string info;

	string depthName = "unknown";
	switch(I.depth()) {
	case CV_8U : depthName ="8U"; break;
	case CV_8S : depthName ="8S"; break;
	case CV_16U :  depthName ="16U"; break;
	case CV_16S : depthName ="16S"; break;
	case CV_32S : depthName ="32S"; break;
	case CV_32F : depthName ="32F"; break;
	case CV_64F : depthName ="64F"; break;
	case CV_USRTYPE1 : depthName ="USRTYPE1"; break;
	default : cerr << ERROR_LINE << "Unknown type " << I.depth() << endl; exit(-1); break;
	}

	info = cv::format("[%d:%d] %sx%dbands",I.cols,I.rows,depthName.c_str(),I.channels());
	return(info);
}

//要素ごとにmin以下、max以上の値をクリップして返す。
Mat clip(Mat I,int min,int max) {
	Mat L = Mat(I.rows,I.cols,I.type(),min);
	Mat H = Mat(I.rows,I.cols,I.type(),max);
	Mat O;
	O = cv::max(I,L);
	O = cv::min(O,H);
	return(O);
}

//SをDに合った型にしてコピーする。
Mat matchTo(const Mat &S,Mat D) {
	Mat dS; //ビット深度一致
	Mat cS; //チャンネル数一致

	if(S.depth() != D.depth()) {
		S.convertTo(dS,D.depth());
	}
	else { dS = S; }

	if(dS.channels() != D.channels()) {
		if(dS.channels() == 1 && D.channels() == 3) {
			cS = Mat(D.rows,D.cols,D.type());
			cvtColor(dS,cS,CV_GRAY2BGR);
		}
		else {
			cerr << "Error:" << __LINE__ << "error dS.channels()=" << dS.channels() << " D.channels()" << D.channels()  << endl; exit(-1);
		}
	}
	else {
		cS = dS;
	}
	return(cS);
}

//SをD（の部分領域）にコピー。全体コピーの場合はOpenCVが勝手にやってくれる。
void copyTo(const Mat &S,Mat D) {
	if(S.cols != D.cols || S.rows != D.rows) {
		cerr << "Error:" << __LINE__  << "S:" << getImageInfo(S) << " " << "D:" << getImageInfo(D) << endl; exit(-1);
	}
	Mat tS = matchTo(S,D);
	tS.copyTo(D);
}
//SをDの(x1,y1)から始まる部分領域にコピー。
void copyTo(const Mat &S,Mat D,int x1,int y1) {
	copyTo(S,D(Range(y1,y1+S.rows),Range(x1,x1+S.cols)));
}

void resizeTo2(const Mat &S,Mat D,int interpolation) {
	//	cout << "#" << __func__ << " " << S.size() << " " << D.size() << endl;
	if(interpolation == MYCV_INTER_AUTO) {
		if(S.cols <= D.cols && S.rows <= D.rows) {
			interpolation = CV_INTER_NN;
		}
		else { interpolation = CV_INTER_LINEAR; }
	}

	Mat tS = matchTo(S,D);
	resize(tS,D,D.size(),0.0,0.0,interpolation);
}

template <class T>
Mat_<T> getSubImage(Mat_<T> M,int x1,int y1,int w,int h) {
	//x1,y1から始まるwxhの領域を指す部分行列を返す。
	//	printf("%d %d %d %d\n",M.cols,M.rows,y1,h);
	return(M(Range(y1,y1+h),Range(x1,x1+w)));
}

Mat getYImage(Mat bgrImage) {
	Mat ycrcbImage;
	vector<Mat> ycrcb;
	cvtColor(bgrImage,ycrcbImage,CV_BGR2YCrCb);
	split(ycrcbImage,ycrcb);

	return(ycrcb[0]);
}

void normalizeChannels(const Mat& src, Mat& dst,
		double alpha, double beta, int normType, int rtype, const Mat& mask) {
	//cv::normalize()は全面の最大値・最小値で正規化してしまうので
	//各面それぞれ最大・最小で正規化するために作成。

	if(src.channels() <= 1) {
		cerr << __func__ << " " << __LINE__ << "src.channels() <= 1.  use cv::normalize()" << endl; exit(-1);
	}

	vector< Mat > srcv;
	split(src,srcv);
	for(unsigned int i = 0; i < srcv.size(); i++) {
		//	double min,max;
		//		minMaxLoc(srcv[i],&min,&max);
		//		cout << i << " " << min << " " << max << " " << min + 1 << " " << log(min+1) << endl;
		normalize(srcv[i],srcv[i],alpha,beta,normType,rtype,mask);
	}

	merge(srcv,dst);
}


template <class T>
Mat normalizeNSigma(Mat_<T> M,int type,double nSigma) {
	//2次元行列Mを平均値±nSigma×標準偏差でtypeで決まる範囲に正規化する。
	int halfValue = 0;
	switch(type) {
	case CV_8U :
		halfValue = 128;
		break;
	case CV_16U:
		halfValue = 32768;
		break;
	default : CV_Error(-1,"Unknown type \n"); exit(0);
	}
	IplImage iplM = M;
	CvScalar mean;
	CvScalar std_dev;
	cvAvgSdv(&iplM,&mean,&std_dev);
	double med = mean.val[0];
	double max = std_dev.val[0]*nSigma;
	double alpha = (halfValue-1)/max;
	double beta  = halfValue*(1-med/max);
	errorMessage("%f %f %f %f\n",med,max,alpha,beta);
	//	cout << alpha << " " << beta << endl;
	Mat M8U;
	M.convertTo(M8U,CV_8U,alpha,beta);
	return(M8U);
}

template <class T>
Mat createMultiLevelImage(Mat_<T> *Ms,int nLevel,double nSigma) {
	//画像ピラミッドを表示用に8bit階調に平均値±標準偏差×nSigmaの正規化し、
	//横/縦（未実装）一列に配置した画像を作成して返す。
	//T :入力画素値の型
	//Ms : グレー画像ピラミッド。
	//nSigma : 正規化範囲。nSigma<-の場合、正規化を行わない
	int width = 0;
	int height = Ms[0].rows;
	for(int l = 0; l < nLevel; l++) {
		width = width + Ms[l].cols;
	}

	//	Mat_<T> mM(height,width);
	Mat mM = Mat_<T>(height,width);
	int x1 = 0;
	int y1 = 0;
	for(int l = 0; l < nLevel; l++) {
		y1 = height - Ms[l].rows;
		Mat nM;
		if(nSigma > 0) {
			nM = normalizeNSigma<T>(Ms[l],CV_8U,nSigma);
		}
		else {
			nM = Ms[l];
		}
		//		mycv::copyTo(nM,mM(Range(y1,y1+nM.rows),Range(x1,x1+nM.cols)));//x1,y1);
		mycv::copyTo(nM,Mat(mM(Range(y1,y1+nM.rows),Range(x1,x1+nM.cols))));//x1,y1);
		//		cout << x1 << " " << y1 << endl;
		x1 = x1 + Ms[l].cols;
	}

	Mat mM8U;
	mM.convertTo(mM8U,CV_8U);

	return(mM8U);
}

//3Channel画像IをC0,C1,C2に分離
void split3Channels(Mat I,Mat &C0,Mat &C1,Mat &C2) {
	vector< Mat > vI;
	if(I.channels() != 3) {
		cerr << ERROR_LINE << " I.channels() != 3)" << endl; exit(-1);
	}
	split(I,vI);
	C0 = vI[0];
	C1 = vI[1];
	C2 = vI[2];
}
//1Channel画像C0,C1,C2を3Channel画像Oに連結
void merge3Channels(Mat C0,Mat C1,Mat C2,Mat &O) {
	if(C0.channels() != 1 || C1.channels() != 1 || C2.channels() != 1) {
		cerr << ERROR_LINE << " C0.channels() != 1 || C1.channels() != 1 || C2.channels() != 1" << endl; exit(-1);
	}
	vector< Mat > vO;
	vO.push_back(C0);
	vO.push_back(C1);
	vO.push_back(C2);
	merge(vO,O);
}



template <class T>
Mat_<T> *splitBayer(Mat_<T> B) {
	Mat_<T> *P = new Mat_<T>[4];

	int w = B.cols/2;
	int h = B.rows/2;

	for(int b = 0; b <= 3; b++) {
		P[b] = Mat(h,w,B.type());
		//		printf("%d %d\n",P[b].cols,P[b].rows);
	}

	int tx,ty;
	for(int y = 0; y < B.rows; y = y+2) {
		for(int x = 0; x < B.cols; x = x+2) {
			tx = x/2;
			ty = y/2;
			P[0](ty,tx) = B(y  ,x  );
			P[1](ty,tx) = B(y  ,x+1);
			P[2](ty,tx) = B(y+1,x  );
			P[3](ty,tx) = B(y+1,x+1);
		}
	}

	return(P);
}

Mat_<unsigned short> convertToBayerImage(Mat I) {
	//符号なし16bitRGB画像をRGrGbBベイヤー配列に変換する。
	if(I.type() != CV_16UC3) {
		cerr << ERROR_LINE << "I.type() != CV_16UC3" << endl; exit(-1);
	}
	vector< Mat_<unsigned short> > vI;
	split(I,vI);
	Mat_<unsigned short> B(I.rows/2*2,I.cols/2*2,(unsigned short)0);
	for(int y = 0; y < B.rows; y = y + 2) {
		for(int x = 0; x < B.cols; x = x + 2) {
			B(y  ,x  ) = vI[2](y  ,x  ); //R
			B(y  ,x+1) = vI[1](y  ,x+1); //Gr
			B(y+1,x  ) = vI[1](y+1,x  ); //Gb
			B(y+1,x+1) = vI[0](y+1,x+1); //B
		}
	}

	return(B);
}
Mat_<unsigned short> shrinkBayer3x3(Mat_<unsigned short> B) {
	Mat_<unsigned short> rB((B.rows-4)/3/2*2,(B.cols-4)/3/2*2); //端画素は省略
	int tx,ty;
	long sum;
	rB = 0;
	for(int y = 0; y < rB.rows; y++) {
		for(int x = 0; x < rB.cols; x++) {
			tx = 2 + 3*x;
			ty = 2 + 3*y;
			//rB(y,x) = B(ty,tx);
			sum =   B(ty-2,tx-2) + B(ty-2,tx  ) + B(ty-2,tx+2) +
					B(ty  ,tx-2) + B(ty  ,tx  ) + B(ty  ,tx+2) +
					B(ty+2,tx-2) + B(ty+2,tx  ) + B(ty+2,tx+2);
			rB(y,x) = sum / 9;
		}
	}
	//		cout << getImageInfo(rB) << endl;
	//		imwrite("rB.tif",rB);
	//
	return(rB);
}


Mat_<float> *createMeanSigmaMap(Mat I,int m,int n,int p,int q) {
	//mxnウィンドウの平均・分散を1/px1/qの画像化
	int w = I.cols/p;
	int h = I.rows/q;
	Mat_<float> M = Mat(h,w,CV_32F); // Mean Map
	Mat_<float> S = Mat(h,w,CV_32F); // Sigma Map
	Mat_<float> N = Mat(h,w,CV_32F); // Noise Map
	Mat L; //Local patch

	//   m1 c  m2
	//  ----*-----
	int m1 = (m-1)/2;
	//	int m2 = m/2;
	int n1 = (n-1)/2;
	//	int n2 = n/2;
	int is,js,ie,je;
	is = (int)ceil((float)m1/p);
	js = (int)ceil((float)n1/q);
	ie = (int)floor(((float)I.cols-m+m1)/p);
	je = (int)floor(((float)I.rows-n+n1)/q);
	//	printf("%d %d %d %d %d %d %d %d  %d %d\n",I.cols,I.rows,w,h,is,ie,js,je,m1,n1);
	int x1,y1;
	for(int j = js; j < je-1; j++) {
		for(int i = is; i < ie-1; i++) {
			Scalar means;
			Scalar std_devs;
			x1 = i*p-m1;
			y1 = j*q-n1;
			//			errorMessage("%4d %4d %4d %4d  %4d %4d\n",i,j,x1,y1,x1+m,y1+n);
			//			x1 = i*m;
			//			y1 = j*n;
			L = I(Range(y1,y1+n),Range(x1,x1+m));
			meanStdDev(L,means,std_devs);
			//			cvAvgSdv(&L,&means,&std_devs);
			M(j,i) = (float)means.val[0];
			S(j,i) = (float)std_devs.val[0];
			N(j,i) = (S(j,i)*S(j,i))/M(j,i);
			//			printf("%4d %4d  %12f %12f\n",x1,y1,M(j,i),S(j,i));
		}
	}
	//exit(0);
	Mat_<float> *P = new Mat_<float>[3];
	P[0] = M;
	P[1] = S;
	P[2] = N;
	return(P);
}

template <class T>
T pool(Mat_<T> &S,const Mat_<T> &W,int margin) {
	//	errorMessageExit("%d\n",margin);
	Mat_<T> w;
	if(W.cols == 0 || W.rows == 0) {
		w = Mat::ones(Size(S.cols,S.rows),CV_64F);
	}
	else { w = W;}

	//	printf("%dx%d  %dx%d\n",S.cols,S.rows,w.cols,w.rows);
	CV_Assert(S.cols == w.cols && S.rows == w.rows);

	T value = 0;
	T wSum = 0.0;

	for(int y = margin; y < S.rows-margin; y++) {
		for(int x = margin; x < S.cols-margin; x++) {
			if(S(y,x) != S(y,x)) {
				errorMessage("NaN %d %d %f\n",x,y,S(y,x));
				continue;
			}
			//			CV_Assert(S(y,x) != nan);
			value = value + w(y,x)*S(y,x);
			wSum = wSum + w(y,x);
		}
	}
	value = value / wSum;
	//	errorMessage("%f %f\n",value,wSum); exit(0);

	return(value);
}

vector<Rect> createAreas(Mat m,int w,int h,Rect evalArea,int xStep,int yStep) {
	//vector<Rect> createAreas(Mat m,int w,int h,Rect evalArea=Rect(),int xStep=-1,int yStep=-1) {
	//Mat m内に設定されるwxh矩形領域のリストを返す。
	vector<Rect> areas;

	if(evalArea == Rect()) { evalArea = Rect(0,0,m.cols,m.rows); }
	if(xStep <= 0) { xStep = w; }
	if(yStep <= 0) { yStep = h; }

	//	cout << evalArea << endl;
	//	cout << w <<" "<< h <<" "<< xStep <<" "<< yStep << endl;
	for(int y = evalArea.y; y <= evalArea.y + evalArea.height - h; y = y + yStep) {
		for(int x = evalArea.x; x <= evalArea.x + evalArea.width - w; x = x + xStep) {
			areas.push_back(Rect(x,y,w,h));
		}
	}
	//	cout << areas[0] << endl;

	return(areas);
}

template <class TS,class TD>
void resizeTo(const Mat &S,Mat_< TD > &D,Rect rect,int interpolation){
	//SをDの部分領域rectにコピーする。
	//interpolation==mycv:MYCV_INTER_AUTOの場合、拡大であればNearestNeighbor/それ以外は補間
	//TS,TDが同じ場合
	if(interpolation == MYCV_INTER_AUTO) {
		if(S.cols <= rect.width && S.rows <= rect.height) {
			interpolation = CV_INTER_NN;
		}
		else { interpolation = CV_INTER_LINEAR; }
	}
	cv::resize(S,D(rect),Size(rect.width,rect.height),0,0,interpolation);
}
template void resizeTo<Vec3b,Vec3b>(const Mat &S,Mat_< Vec3b > &D,Rect rect,int interpolation);

//S,Dのdepth,nChannelが異なる場合のための特殊化
template<> void resizeTo<uchar,Vec3b>(const Mat &S,Mat_< Vec3b > &D,Rect rect,int interpolation){
	Mat_<Vec3b> tS;
	cvtColor(S,tS,CV_GRAY2BGR);
	mycv::resizeTo<Vec3b,Vec3b>(tS,D,rect,interpolation);
}
template<> void resizeTo<short,Vec3b>(const Mat &S,Mat_< Vec3b > &D,Rect rect,int interpolation){
	Mat_<uchar> tS = S;//cvtColorは(depth == CV_8U || depth == CV_16U || depth == CV_32F)以外入力できないので変換。
	mycv::resizeTo<uchar,Vec3b>(tS,D,rect,interpolation);
}

Scalar lambdaToBGR(int lambda_nm,int l1,int l2,int h1,int h2) {
	//HSVのH=h1からh2までを波長l1からl2に割り当てたBGR値をlambda_nmについて生成して返す。

	int lx;
	if(lambda_nm >= l1) { lx = h1; }
	else if(lambda_nm <= l2) { lx = h2; }
	else { lx = (lambda_nm - l2)*(h1-h2)/(l1-l2) + h2; }

	Mat_<Vec3b> T(1,1);
	T = Vec3b(lx/2,255,255);
	cvtColor(T,T,CV_HSV2BGR);
	Scalar bgr(T(0,0)[0],T(0,0)[1],T(0,0)[2]);
	return(bgr);
}

//テンプレート変数で条件分岐するための構造体
template < class T > struct getCVDepth { static const int value = -1; };
template <> struct getCVDepth<unsigned short> { static const int value = CV_16U; };
template <> struct getCVDepth<short> { static const int value = CV_16S; };
template <> struct getCVDepth<float> { static const int value = CV_32F; };

template < class T >
void writeTiff16bitRGB(string name,Mat I,int compression) {
#ifdef VS2012
	cout << ERROR_LINE << "Not implemented yet. for VS2012" << endl;
#else
	int type = CV_MAKETYPE(getCVDepth<T>::value,3);
	if(I.type() != type) {
		cerr << ERROR_LINE << "I.type():" << I.type() << " != " << type << endl; exit(-1);
	}

	unsigned int depth = 16;
	unsigned int nplanes = 3;
	unsigned int width = I.cols;
	unsigned int height = I.rows;

	Mat L = min(I,(1<<16)-1);
	L = max(L,0);

	vector< Mat_<T> > Lv;
	split(L,Lv);

	uint16* buf = (uint16*)calloc( width * height * 3, sizeof(uint16));
	int k;
	for(unsigned int y=0; y < height; y++){
		for(unsigned int x=0;x < width; x++){
			k = x*nplanes + y*width*nplanes;
			buf[k + 0] = Lv[2](y,x); //R
			buf[k + 1] = Lv[1](y,x); //G
			buf[k + 2] = Lv[0](y,x); //B
			//			if(y == 0) { cout << cv::format("%d %d %d %d\n",x, y, buf[k+0], Lv[2](y,x)); }
		}
	}

	TIFF *tif;

	// TIFFファイルを開く
	if((tif = TIFFOpen(name.c_str(), "w")) == NULL){
		cerr << ERROR_LINE << "Can't open file. " << name << endl; exit(-1);
	}

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     height);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   depth);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, nplanes);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);//PLANARCONFIG_SEPARATEだと上手く機能しない？
	TIFFSetField(tif, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);// 0->白:PHOTOMETRIC_MINISWHITE 0:->黒:PHOTOMETRIC_MINISBLACK RGB:PHOTOMETRIC_RGB
	TIFFSetField(tif, TIFFTAG_COMPRESSION,     compression);// 圧縮形式。

	TIFFWriteEncodedStrip(tif, 0, buf, width * height * sizeof(uint16)*3); //これでいけた？

	//TIFFファイルのメモリを解放します。
	TIFFClose(tif);
	free(buf);
#endif
}
template void writeTiff16bitRGB<unsigned short>(string name,Mat I,int compression);
template void writeTiff16bitRGB<short>(string name,Mat I,int compression);
template void writeTiff16bitRGB<float>(string name,Mat I,int compression);

//プロット表示用の色を計算する？
Scalar getColor(unsigned char v,unsigned char s,int i,int n) {
	unsigned char h = round((360-45)/2*i/n);
	Mat_<Vec3b> t(1,1,Vec3b(h,s,v));
	//		cout << t << endl;
	cvtColor(t,t,CV_HSV2BGR);

	//		cout << t << endl;
	Vec3b c = t(0,0);
	Scalar color(c[0],c[1],c[2]);
	//		cout << color << endl;

	return(color);
}

//Matに文字列を書き込む
void TextWriter::put(Mat I,string text,Point pt,Scalar fg,Scalar bg,int bgWidth) {
	Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
	textWidth = textSize.width;
	textHeight = textSize.height + baseLine;

	if(bgWidth < 0) { bgWidth = textWidth; }
	int bgHeight = textHeight;
	fg = (fg == Scalar()) ? CV_BLACK : fg;
	//bg = (bg == Scalar()) ? CV_WHITE : bg;

	if(autoColor == true) {
		Mat t = I(Range(pt.y,pt.y+textWidth),Range(pt.x,pt.x+textHeight)).clone();
		Scalar mmm,sss;
		cvtColor(t,t,CV_BGR2YCrCb);
		meanStdDev(t,mmm,sss);
		//		cout << mmm << " ";
		mmm.val[0] = mmm.val[0] > 128 ? 0 : 255;
		mmm.val[1] = mmm.val[2] = 0;

		Mat_<Vec3f> tt(1,1,Vec3f((float)mmm.val[0],(float)mmm.val[1],(float)mmm.val[2]));
		cvtColor(tt,tt,CV_YCrCb2BGR);
		fg.val[0] = tt(0,0).val[0];
		fg.val[1] = tt(0,0).val[1];
		fg.val[2] = tt(0,0).val[2];
	}
	//cout << bg << endl;
	if(bg != Scalar()) {
		rectangle(I,pt,Point(pt.x+bgWidth,pt.y+bgHeight),bg,-1);
	}
	putText(I,text,Point(pt.x,pt.y + textHeight - baseLine),fontFace,fontScale,fg,thickness);
}
void TextWriter::describe(Mat I, string text, Scalar fg, Scalar bg, int bgWidth) {
	Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
//	textHeight = textSize.height + baseLine;
//	Point pt(10, I.rows - textHeight);
	Point pt(5, 5);
	put(I, text, pt, fg, bg, bgWidth);
}
//Matに文字列を書き込む
void TextWriter::putV(Mat I,string text,Point pt,Scalar fg,Scalar bg,int bgWidth) {
	Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
	textWidth = textSize.width;
	textHeight = textSize.height + baseLine;

	if(bgWidth < 0) { bgWidth = textWidth; }
//	int bgHeight = textHeight;
	fg = (fg == Scalar()) ? CV_BLACK : fg;
	//bg = (bg == Scalar()) ? CV_WHITE : bg;

	if(autoColor == true) {
		Mat t = I(Range(pt.y,pt.y+textWidth),Range(pt.x,pt.x+textHeight)).clone();
		Scalar mmm,sss;
		cvtColor(t,t,CV_BGR2YCrCb);
		meanStdDev(t,mmm,sss);
		//		cout << mmm << " ";
		mmm.val[0] = mmm.val[0] > 128 ? 0 : 255;
		mmm.val[1] = mmm.val[2] = 0;

		Mat_<Vec3f> tt(1,1,Vec3f((float)mmm.val[0],(float)mmm.val[1],(float)mmm.val[2]));
		cvtColor(tt,tt,CV_YCrCb2BGR);
		fg.val[0] = tt(0,0).val[0];
		fg.val[1] = tt(0,0).val[1];
		fg.val[2] = tt(0,0).val[2];
	}
	Mat textImage(textHeight,textWidth,CV_8UC3,bg);
//	if(bg != Scalar()) {
//		textImage = bg;
////		rectangle(I,pt,Point(pt.x+bgWidth,pt.y+bgHeight),bg,-1);
//	}
	putText(textImage,text,Point(0,textHeight - baseLine),fontFace,fontScale,fg,thickness);
	textImage = textImage.t();
	flip(textImage,textImage,CV_FLIPCODE_X);
	copyTo(textImage,I,pt.x,pt.y);
}


//! Write cv::Mat as binary
/*!
\param[out] ofs output file stream
\param[in] out_mat mat to save
 */
bool writeMatBinary(std::ofstream& ofs, const cv::Mat& out_mat) {
	if(!ofs.is_open()){
		return false;
	}
	if(out_mat.empty()){
		int s = 0;
		ofs.write((const char*)(&s), sizeof(int));
		return true;
	}
	int type = out_mat.type();
	ofs.write((const char*)(&out_mat.rows), sizeof(int));
	ofs.write((const char*)(&out_mat.cols), sizeof(int));
	ofs.write((const char*)(&type), sizeof(int));
	ofs.write((const char*)(out_mat.data), out_mat.elemSize() * out_mat.total());

	return true;
}

//! Save cv::Mat as binary
/*!
\param[in] filename filaname to save
\param[in] output cvmat to save
 */
bool saveMatBinary(const std::string& filename, const cv::Mat& output){
	std::ofstream ofs(filename.c_str(), std::ios::binary);
	return writeMatBinary(ofs, output);
}


//! Read cv::Mat from binary
/*!
\param[in] ifs input file stream
\param[out] in_mat mat to load
 */
bool readMatBinary(std::ifstream& ifs, cv::Mat& in_mat) {
	if(!ifs.is_open()){
		return false;
	}

	int rows, cols, type;
	ifs.read((char*)(&rows), sizeof(int));
	if(rows==0){
		return true;
	}
	ifs.read((char*)(&cols), sizeof(int));
	ifs.read((char*)(&type), sizeof(int));

	in_mat.release();
	in_mat.create(rows, cols, type);
	ifs.read((char*)(in_mat.data), in_mat.elemSize() * in_mat.total());

	return true;
}


//! Load cv::Mat as binary
/*!
\param[in] filename filaname to load
\param[out] output loaded cv::Mat
 */
bool loadMatBinary(const std::string& filename, cv::Mat& output){
	std::ifstream ifs(filename.c_str(), std::ios::binary);
	return readMatBinary(ifs, output);
}

template Mat_<unsigned char> getSubImage(Mat_<unsigned char>,int,int,int,int);
template Mat_<short> getSubImage(Mat_<short>,int,int,int,int);
template Mat_<float> getSubImage(Mat_<float>,int,int,int,int);
template Mat_<double> getSubImage(Mat_<double>,int,int,int,int);
template Mat_< Vec3b > getSubImage(Mat_< Vec3b >,int,int,int,int);
template Mat_< Vec3f > getSubImage(Mat_< Vec3f >,int,int,int,int);
template Mat normalizeNSigma(Mat_<double> ,int,double);
template Mat normalizeNSigma(Mat_<float> ,int,double);
template Mat createMultiLevelImage(Mat_<double> *,int,double);
template Mat createMultiLevelImage(Mat_<float> *,int,double);
template float pool(Mat_<float> &S,const Mat_<float> &W,int margin);
template double pool(Mat_<double> &S,const Mat_<double> &W,int margin);
template Mat_<unsigned short>* splitBayer(Mat_<unsigned short>);
//template Mat_<double>* createMeanSigmaMap(Mat,int,int);

} /* namespace matutils */
