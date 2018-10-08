/*
 * MatUtils.hpp
 *
 *	OpenCVのMatを用いた画像処理のための共通関数
 *
 */

#ifndef MATUTILS_HPP_
#define MATUTILS_HPP_

//#include <tiff.h>
#define	    COMPRESSION_NONE		1
#define int64 opencv_broken_int64
#define uint64 opencv_broken_uint64
#include <opencv2/opencv.hpp>
#undef int64
#undef uint64

using namespace cv;
using namespace std;

namespace mycv {
//文字/線に良く使う色の定義
const Scalar CV_BLACK 		= CV_RGB(  0,  0,  0); //Scalar()と区別できない
const Scalar CV_BLACK1 		= CV_RGB(  1,  1,  1);
const Scalar CV_DARKGRAY 	= CV_RGB( 64, 64, 64);
const Scalar CV_GRAY 		= CV_RGB(128,128,128);
const Scalar CV_LIGHTGRAY 	= CV_RGB(196,196,196);
const Scalar CV_WHITE 		= CV_RGB(255,255,255);
const Scalar CV_RED 		= CV_RGB(255,  0,  0);
const Scalar CV_GREEN		= CV_RGB(  0,255,  0);
const Scalar CV_BLUE 		= CV_RGB(  0,  0,255);
const Scalar CV_CYAN 		= CV_RGB(  0,255,255);
const Scalar CV_MAGENTA		= CV_RGB(255,  0,255);
const Scalar CV_YELLOW		= CV_RGB(255,255,  0);

//角度とラジアンを変換するマクロ
#define TO_RADIAN(x) ((x)*M_PI/180.0)
#define TO_DEGREE(x) ((x)/M_PI*180.0)

//ウインドウのタイトルと枠の長さ。QuickLookのウインドウを並べて配置する時のステップに加算
const int wMargin = 8;
const int wTitleMargin = 22;

//cv::flip()で指定するflipCode。定義されていなかったので。
const int CV_FLIPCODE_X = 1;
const int CV_FLIPCODE_Y = 0;
const int CV_FLIPCODE_BOTH = -1;

//OpenCVの色を表すScalarをMat_<Vec3b>の画素値Vec3bに変換する。
inline Vec3b toVec3b(Scalar color) {
	return(Vec3b((uchar)color.val[0],(uchar)color.val[1],(uchar)color.val[2]));
}
//min以下、max以上の値をクリップして返す。
inline float clip(float value,int min,int max) {
	return(value < min ? min : (value > max ? max : value));
}
//min以下、max以上の値をクリップして返す。
inline int clip(int value,int min,int max) {
	return(value < min ? min : (value > max ? max : value));
}
//min以下、max以上の値をクリップして返す。
inline unsigned int clip(unsigned int value,unsigned int min,unsigned int max) {
	return(value < min ? min : (value > max ? max : value));
}

//要素ごとにmin以下、max以上の値をクリップして返す。
Mat clip(Mat I,int min,int max);

//Matの基本情報を表示。読込確認用。
string getImageInfo(Mat I);

//SをDに合ったサイズと型にしてコピーする。
Mat matchTo(const Mat &S,Mat D);
void copyTo(const Mat &S,Mat D); //S,Dの型の違いを中で解決する。Dを参照にすると呼び出し時に一時変数を指定できない。
void copyTo(const Mat &S,Mat D,int x1,int y1);

enum {//CVの値と重複しないように。cv側は名前空間の外で定義されているのでcv::付けるとエラーになる。
	CV_INTER_NN=CV_INTER_NN,CV_INTER_LINEAR=CV_INTER_LINEAR,CV_INTER_CUBIC=CV_INTER_CUBIC,
	CV_INTER_AREA=CV_INTER_AREA,CV_INTER_LANCZOS4=CV_INTER_LANCZOS4,
	MYCV_INTER_AUTO};
template <class TS,class TD>
void resizeTo(const Mat &S,Mat_< TD > &D,Rect rect,int interpolation=MYCV_INTER_AUTO);
void resizeTo2(const Mat &S,Mat D,int interpolation=MYCV_INTER_AUTO); //S,Dの型の違いを中で解決する。Dを参照にすると呼び出し時に一時変数を指定できない。

template <class T>
Mat_<T> getSubImage(Mat_<T> M,int x1,int y1,int w,int h);

Mat getYImage(Mat rgb);

//template <class T>
//void copyTo(const Mat_<T> &S,Mat_<T> &D,int x1,int y1) {
//	//Sをx1,y1から始まるDの部分行列にコピーする。
//	S.copyTo(D(Range(y1,y1+S.rows),Range(x1,x1+S.cols)));
//}

//cv::normalize()は全面の最大値・最小値で正規化してしまうので
//各面それぞれ最大・最小で正規化するために作成。
void normalizeChannels(const Mat& src, Mat& dst,
		double alpha=1, double beta=0, int normType=NORM_L2, int rtype=-1, const Mat& mask=Mat());

template <class T>
Mat normalizeNSigma(Mat_<T> M,int type,double nSigma=3);

template <class T>
Mat createMultiLevelImage(Mat_<T> *Ms,int nLevel,double nSigma=3);

void split3Channels(Mat I,Mat &C0,Mat &C1,Mat &C2);
void merge3Channels(Mat C0,Mat C1,Mat C2,Mat &O);

enum InputImageType { UNKNOWN,RGB8bit,CRGB12bit,CRGB14bit,Bayer12bit,Bayer14bit};

template <class T>
Mat_<T> *splitBayer(Mat_<T> B);
Mat_<unsigned short> convertToBayerImage(Mat I);
Mat_<unsigned short> shrinkBayer3x3(Mat_<unsigned short> B);
Mat_<float> *createMeanSigmaMap(Mat I,int m,int n,int p,int q);

template <class T>
T pool(Mat_<T> &S,const Mat_<T> &W=Mat_<T>(),int margin=0);

//vector<Rect> createAreas(Mat m,int w,int h,Rect evalArea,int xStep,int yStep);
vector<Rect> createAreas(Mat m,int w,int h,Rect evalArea=Rect(),int xStep=-1,int yStep=-1);

Scalar lambdaToBGR(int lambda_nm,int l1=640,int l2=380,int h1=0,int h2=300);

template < class T >
void writeTiff16bitRGB(string name,Mat I,int compression=COMPRESSION_NONE);

//bool writeMatBinary(std::ofstream& ofs, const cv::Mat& out_mat);
bool saveMatBinary(const std::string& filename, const cv::Mat& output);
//bool readMatBinary(std::ifstream& ifs, cv::Mat& in_mat);
bool loadMatBinary(const std::string& filename, cv::Mat& output);

Scalar getColor(unsigned char v,unsigned char s,int i,int n);

//Matに文字列を書き込むためのクラス。
class TextWriter {
	int fontFace;
	int thickness;
public:
	float fontScale;
	int baseLine;
	int textWidth;
	int textHeight;
	bool autoColor;
	TextWriter(float fontScale=0.6,bool autoColor=false) {
		fontFace = CV_FONT_HERSHEY_SIMPLEX;
		this->fontScale = fontScale;
		thickness = 1;
		string text = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,?#@";
		Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
		textWidth = textSize.width;
		textHeight = textSize.height + baseLine;
		this->autoColor=autoColor;
	}
	~TextWriter() {

	}

	int getTextWidth(string text) {
		Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
		return(textSize.width);
	}
	int getTextHeight(string text) {
		Size textSize = getTextSize(text,fontFace,fontScale,thickness,&baseLine);
		return(textSize.height);
	}

	void put(Mat I, string text, Point pt, Scalar fg = Scalar(), Scalar bg = Scalar(), int bgWidth = -1);
	void putV(Mat I, string text, Point pt, Scalar fg = Scalar(), Scalar bg = Scalar(), int bgWidth = -1);
	void describe(Mat I, string text, Scalar fg = Scalar(), Scalar bg = Scalar(), int bgWidth = -1);
	Point next(Point pt) {
		return(Point(pt.x,pt.y+textHeight));
	}
};

////Mat pow(Mat src,float p);
//cv::pow()が32F/64Fのdepthにしか対応していないが、下のようにすれば良い。
//Mat_<Vec3f> fsI = Mat_<Vec3f>(sI); pow(fsI/calcMax,1.0/gamma,fsI);

} /* namespace mycv */
#endif /* MATUTILS_HPP_ */
