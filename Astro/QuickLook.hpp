/*
 * QuickLook.hpp
 *
 * 画像表示確認用のクラス
 * 画像のグループを同じ場所・倍率で比較表示するためのUIを備える。
 */
//TODO 画像ごとに異なる場所を表示?。

#ifndef QUICKLOOK_HPP_
#define QUICKLOOK_HPP_

#include <opencv2/opencv.hpp>

#include "MatUtils.hpp"

using namespace cv;
using namespace std;

namespace mycv {
//Cygwin32bit OpenCV 2.4.7の時の値
//int VK_LEFT = 81;
//int VK_RIGHT = 83;
//C:\cygwin64\tmp\opencv-2.4.11\modules\highgui\src\window_w32.cppが返す値
//#define VK_LEFT  2424832 //(0x25 << 16); //switchで使うために定数化
//#define VK_RIGHT 2555904 //(0x27 << 16); //

//enum VirtualKey {KEY_LEFT=81,KEY_RIGHT=83,KEY_TAB=0x09,KEY_ESC=0x1B};
#ifdef VK_QT
enum VirtualKey {KEY_LEFT = 65361, KEY_RIGHT = 65363, KEY_UP = 65362, KEY_DOWN = 65364, KEY_TAB = 0x09, KEY_ESC = 0x1B};
#else
enum VirtualKey {KEY_LEFT = 2424832, KEY_RIGHT = 2555904, KEY_UP = 2490368, KEY_DOWN = 2621440, KEY_TAB = 0x09, KEY_ESC = 0x1B};
#endif

class QuickLook {
public:
	enum Mode {ARRANGE, STACK};
private:
	enum Direction{H, V};

	Mode mode;
public:
	int wx0; //QuickLook表示領域の画面に対する原点
	int wy0; //QuickLook表示領域の画面に対する原点
	int wx;  //次の画像表示位置の基準
	int wy;  //次の画像表示位置の基準
	int wyn;  //次の画像表示位置の基準
	int wxMargin; //ウインドウ枠
	int wyMargin; //ウインドウ枠

public:
	string windowName;
	vector<string> names;
	vector<Mat> Ms; //元データ
	vector<Mat_<Vec3b> > Ds; //表示用
	vector<Point> wps; //ARRANGEモードの表示位置
	Point ws;
	int wxn;  //次の画像表示位置の基準
	int iCurrentWindow;
	int nWindow;
	int mx;
	int my;
	bool verbose;

	QuickLook();
	QuickLook(Mode mode, string windowName = "window1"); //全部の変数に初期値があると引数なしコンストラクタと区別ができないと怒られる。そらそうか。
	virtual ~QuickLook();
	void initialize(Mode mode = ARRANGE, string windowName = "window1", int wx = 0,int wy = 0);
	void initializeArrangeMode(string windowName, int wx = 0, int wy = 0);
	void initializeStackMode(string windowName, int wx = 0, int wy = 0);

	Mat_<Vec3b> convert(Mat M);
	void add(string name, Mat M, int x, int y, int scale = 1, int waitTime = 0);
	void add(string name, Mat M, Direction d = H, int scale = 1, int waitTime = 0);
	void addH(string name, Mat M, int scale = 1, int waitTime = 0);
	void addV(string name, Mat M, int scale = 1, int waitTime = 0);
	void addS(string name, Mat M, int scale = 1, int waitTime = 0);
	void addS(Mat M, int scale = 1, string description = "", int waitTime = 0);
	void setPosition(int wx, int wy);
	void setPositionNext();
	void setPositionNextH();
	void setPositionNextV();
	void wait();
	void list();
	void next();
	void prev();
	void setImage(string windowName,int i);

private:
//	void cbMousePrintWindowName(int event,int x,int y,int flag,void *data);

}; //class QuickLook

typedef QuickLook QL;

} //namespace mycv
#endif /* QUICKLOOK_HPP_ */
