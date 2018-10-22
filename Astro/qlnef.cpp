/*
 * qlnef.cpp
 *
 */

#include <iostream>
#include <fstream>
#include <string>

#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.hpp"
#include "MatUtils.hpp"
#include "QuickLook.hpp"

using std::string;
using std::cout;
using std::endl;

using namespace cv;
using namespace mycv;

#include "Astro.hpp"

DEFINE_string(i, "unknown.nef", "Input NEF file.");
DEFINE_string(flat, "flat.tif", "Flat file.");

DEFINE_int32(D, 0, "Display scale");
DEFINE_int32(Dt, -8, "Display thumbnail scale");
DEFINE_double(Ds, 3.0, "Display sigma");
DEFINE_int32(H, 0, "Display histogram");
DEFINE_double(gamma, 2.2, "Display gamma");

//コールバック関数はメンバ関数にできない?
//cb()でクリックDownによりボックス描画モードに入って、
//モード中はwait()で一時画像に矩形を描写・更新
//cb()でクリックUpで描画モードから抜けて、確定した矩形を描画・更新

// qlnef -i NEF/20161103-035653\)-0014.nef -flat flat.tif -Ds 3

//-iで指定したNEFを開いて現像
//-fで指定されたフラットでフラット補正
//update()画像中央を初期位置として、枠付きサムネイルと拡大画像を表示
//コールバック関数を設定
//  Mainで指定した位置を中心に移動
//  Thumbnailで指定した位置を中心に移動
//wait()で入力待ち状態に
//		q 終了
//		i 画像処理と表示領域を初期化
//		o 画像処理を初期化
//		r 表示領域を初期化
//		+ scaleを2倍
//		- scaleを1/2倍 してupdate()で枠付きサムネイルと拡大画像を再表示
//		g ガンマ変換をON/OFF
//		,< display_sigmaを-1.0
//		.> display_sigmaを+1.0
//		d エリア指定モードに入る -> エリア内の平均値をダーククリップに設定

//未実装
//		I メタデータを表示
//		D デバッグデータを表示
//		H ヒストグラムを表示
//ヒストグラム場で表示範囲を調整

template < typename T >
class QuickLookNEF {
public:
	QuickLook qlMain;
	QuickLook qlThumbnail;
	QuickLook qlHistogram1;
	QuickLook qlHistogram2;

	bool dark_mode;
	bool drawing_box;
	Rect box;
	Rect dark_box;
	Mat_< Vec< T, 3 > > I1; //フラット補正後の画像
	double min0, max0;

	Mat3b D1_thumbnail; //サムネイル
	Mat3b D1_thumbnail_framed; //サムネイル+枠
	Mat3b D1_main; //メインウインドウ
	Mat3b D1_histogram1; //ヒストグラム
	Mat3b D1_histogram2; //ヒストグラム

	float factor_thumbnail;
	int main_width, main_height;
	int main_xc, main_yc;
	int main_scale;
	float main_factor;

private:
	string input;
	string output;
	string flat;

	Astrophoto A;

	dcraw::NEF nef;
	Mat_< Vec< T, 3 > > I0; //WB補正済みの画像
	Mat_< Vec< T, 1 > > F0; //フラット画像

	Rect rect_awb;
	Scalar mean0, stddev0;
	bool gamma_flag;
	float display_sigma;

	Mat3b D0;
	Mat3b D1; //表示用の元画像

public:
	QuickLookNEF(string _input);
	~QuickLookNEF();

	void initializeImage();
	void setMainPosition(int main_xc, int main_yc, int main_scale);
	void createNormalizedImage();
	void update();

private :
	void wait();
	void createDisplayImages();
};

template < typename T >
void cbMouseThumbnail(int event,int x,int y,int flag,void *data) {
	//cout << __func__ << endl;
	QuickLookNEF<T> *qlnef = reinterpret_cast< QuickLookNEF<T> *>(data);
//	QuickLook *qlThumbnail = &(qlnef->qlThumbnail);
//	QuickLook *qlMain = &(qlnef->qlMain);

	Scalar tmean, tstddev;

	if(qlnef->dark_mode) {
		switch(event) {
		case cv::EVENT_MOUSEMOVE:
			//cout << __func__ << "() EVENT_MOUSEMOVE" << endl;
			if (qlnef->drawing_box){
				qlnef->box.width = x / qlnef->factor_thumbnail - qlnef->box.x;
				qlnef->box.height = y / qlnef->factor_thumbnail - qlnef->box.y;
				//cout << qlnef->box << endl;
				qlnef->update();
			}
			break;
		case cv::EVENT_LBUTTONDOWN:
			//			cout << __func__ << "() EVENT_LBUTTONDOWN" << endl;
			qlnef->drawing_box = true;
			qlnef->box = cv::Rect(x / qlnef->factor_thumbnail, y / qlnef->factor_thumbnail, 0, 0);
			break;
		case cv::EVENT_LBUTTONUP:
			//			cout << __func__ << "() EVENT_LBUTTONUP" << endl;
			qlnef->drawing_box = false;
			qlnef->dark_mode = false;
			if (qlnef->box.width < 0){
				qlnef->box.x += qlnef->box.width;
				qlnef->box.width *= -1;
			}
			if (qlnef->box.height < 0){
				qlnef->box.y += qlnef->box.height;
				qlnef->box.height *= -1;
			}
			cout << "#Selected box : " << qlnef->box << endl;
			//			dark_box = Rect()
			//			cout << cv::format("cbMouseThumbnail lbutton %d %s  %d %d",
			//					qlThumbnail->wxn, qlThumbnail->names[0].c_str(), x, y) << endl;

			meanStdDev(qlnef->I1(qlnef->box), tmean, tstddev);
			qlnef->min0 = tmean.val[1];
			qlnef->createNormalizedImage();
			qlnef->update();
			break;
		default: return; break;
		}
	}
	else {
		switch(event) {
		case EVENT_LBUTTONDOWN :
			//			cout << cv::format("cbMouseThumbnail lbutton %d %s  %d %d",
			//					qlThumbnail->wxn, qlThumbnail->names[0].c_str(), x, y) << endl;
			qlnef->setMainPosition(x / qlnef->factor_thumbnail, y / qlnef->factor_thumbnail, qlnef->main_scale);
			qlnef->update();
			break;
		default: return; break;
		}
	}

	return;
}
template < typename T >
void cbMouseMain(int event,int x,int y,int flag,void *data) {
	QuickLookNEF<T> *qlnef = reinterpret_cast< QuickLookNEF<T> *>(data);
//	QuickLook *qlThumbnail = &(qlnef->qlThumbnail);
//	QuickLook *qlMain = &(qlnef->qlMain);
	int new_xc, new_yc;
	//	string message;
	//cout << flag << endl;
	switch(event) {
	case EVENT_MOUSEMOVE:
		//		qlMain->mx = x;
		//		qlMain->my = y;
		//		message += "MOUSE_MOVE ";
		break;
	case EVENT_LBUTTONDOWN :
		//		cout << cv::format("cbMouseMain lbutton %d %s  %d %d",
		//				qlMain->wxn, qlMain->names[0].c_str(), x, y) << endl;
		new_xc = qlnef->main_xc + (x - qlnef->main_width / 2) / qlnef->main_factor;
		new_yc = qlnef->main_yc + (y - qlnef->main_height / 2) / qlnef->main_factor;
		qlnef->setMainPosition(new_xc, new_yc, qlnef->main_scale);
		qlnef->update();
		break;
	default: return; break;
	}
	//	string *windowName = reinterpret_cast<string *>(data);
	//	printf("%s:%s (%d,%d)\n",windowName->c_str(),message.c_str(),x,y); fflush(stdout);
	return;
}

template < typename T >
QuickLookNEF< T >::QuickLookNEF(string _input) {
	//簡易現像
	//tifを出力
	input = _input;
	flat = FLAGS_flat;
	qlThumbnail.windowName = "Thumnail";
	qlMain.windowName = "Main";
	qlHistogram1.windowName = "Histogram I1";
	qlHistogram2.windowName = "Histogram Main";
	main_width = 512;
	main_height = 512;

	dark_mode = false;
	drawing_box = false;
	box = cv::Rect();

	//-iで指定したNEFを読んで現像
	nef = dcraw::readNEF0(input); //36M,SSDからで0.90s
	cout << nef.getEXIFHeader() << endl;
	cout << nef.getEXIFInfo() << getImageInfo(nef.bayer) << endl;
	double min_value, max_value;
	minMaxLoc(nef.bayer, &min_value, &max_value);
	cout << min_value << " " << max_value << endl;
	I0 = A.bayerToRGB< T >(nef.bayer); //0.15s 36M as float from SDD

	//WB補正
	cout << "#AWB" << endl;
	int w_awb = I0.cols / 4;
	int h_awb = I0.rows / 4;
	int x1_awb = I0.cols / 2 - w_awb / 2;
	int y1_awb = I0.rows / 2 - h_awb / 2;;
	rect_awb = Rect(x1_awb, y1_awb, w_awb, h_awb); //表示用に値を保存
	if(FLAGS_awb) {
		A.AWB(I0, rect_awb); //0.09s 9M
	}

	//フラット補正
	//TODO 関数
	std::ifstream ifs(flat);
	if(ifs.is_open()) {
		cout << "#FlatFielding" << endl;
		F0 = imread(flat.c_str(), -1) / flat_value;
		vector< Mat > vI0;
		split(I0, vI0);
		for(int b = 0; b <= 2; b++) {
			divide(vI0[b], F0, vI0[b]);
		}
		merge(vI0, I1);
	}
	else {
		I1 = I0;
	}
	cv::meanStdDev(I1, mean0, stddev0);
	//表示画像/サムネイル画像を生成
	if(FLAGS_Dt >= 0) {
		cout << "FLAGS_Dt >= 0" << endl; exit(0);
	}
	factor_thumbnail = 1.0 / abs(FLAGS_Dt);

	initializeImage();
	qlMain.wx = D1_thumbnail.cols + qlMain.wxMargin;
	setMainPosition(I1.cols / 2, I1.rows / 2, 1);
	//D1_thumbnail.rows + qlHistogram.wyMargin;
	update();
	//qlThumbnail.wait();

	setMouseCallback(qlThumbnail.windowName, cbMouseThumbnail<T>, this);
	setMouseCallback(qlMain.windowName, cbMouseMain<T>, this);

	wait();

	return;
}
template < typename T >
QuickLookNEF< T >::~QuickLookNEF() {

}

template < typename T >
void QuickLookNEF< T >::setMainPosition(int xc, int yc, int scale) {
	main_xc = xc;
	main_yc = yc;
	main_scale = scale;
}

template < typename T >
void QuickLookNEF< T >::createNormalizedImage() {
	Mat_< Vec< T, 3 > > nI1 = I1.clone();

	double gamma = 1.0;
	if(gamma_flag == true) { gamma = FLAGS_gamma; }
	A.normalize(nI1, min0, max0, gamma);
	D1 = nI1;
	resize(D1, D1_thumbnail, Size(), factor_thumbnail, factor_thumbnail);
	return;
}
template < typename T >
void QuickLookNEF< T >::createDisplayImages() {
	main_factor = 1.0;
	if(main_scale >  1) { main_factor = 1 << (main_scale - 1); }
	if(main_scale < -1) { main_factor = 1.0 / (1 << (abs(main_scale) - 1)); }
	//cout << ERROR_LINE << main_factor << endl;

	int main_x1 = main_xc - main_width / main_factor / 2;
	int main_x2 = main_xc + main_width / main_factor / 2;
	int main_y1 = main_yc - main_height / main_factor / 2;
	int main_y2 = main_yc + main_height / main_factor / 2;
	//cout << ERROR_LINE << main_x1 << " " << main_x2 << " " << main_y1 << " " << main_y2 << endl;

	if(main_x1 < 0) { main_x1 = 0; }
	if(main_x2 > D1.cols) { main_x2 = D1.cols; }
	if(main_y1 < 0) { main_y1 = 0;  }
	if(main_y2 > D1.rows) { main_y2 = D1.rows; }

	D1_thumbnail_framed = D1_thumbnail.clone();
	cv::rectangle(D1_thumbnail_framed,
			Point(main_x1 * factor_thumbnail, main_y1 * factor_thumbnail),
			Point(main_x2 * factor_thumbnail, main_y2 * factor_thumbnail), Scalar(0, 200, 0), 1, 4);
	if(box.width != 0 && box.height != 0) {
		cv::rectangle(D1_thumbnail_framed,
				cv::Point2d(box.x * factor_thumbnail, box.y * factor_thumbnail),
				cv::Point2d((box.x + box.width) * factor_thumbnail, (box.y + box.height) * factor_thumbnail),
				Scalar(0, 200, 200), 1, 4);
	}

	D1_main = D1(Range(main_y1, main_y2), Range(main_x1, main_x2));

	return;
}

template < typename T >
void QuickLookNEF<T>::initializeImage() {
	display_sigma = FLAGS_Ds;
	min0 = mean0.val[1] - 1 * stddev0.val[1];
	max0 = mean0.val[1] + display_sigma * stddev0.val[1];
	gamma_flag = false;
	createNormalizedImage();
	D1_histogram1 = hist(I1, 256, 256);
}

template < typename T >
void QuickLookNEF<T>::update() {
	createDisplayImages();
	D1_histogram2 = hist(D1_main, 256, 256, 8);

	qlThumbnail.addS(D1_thumbnail_framed);
	qlMain.addS(D1_main, main_scale);

	qlHistogram1.wx = D1_thumbnail.cols + qlMain.wxMargin * 2 + D1_main.rows;
	qlHistogram2.wx = qlHistogram1.wx;
	qlHistogram1.wy = qlMain.wy;
	qlHistogram2.wy = qlHistogram1.wy + D1_histogram1.rows + qlHistogram1.wxMargin ;

	cout << qlHistogram1.wx << " " << qlHistogram1.wy << endl;
	cout << qlHistogram2.wx << " " << qlHistogram2.wy << endl;
	qlHistogram1.addS(D1_histogram1);
	qlHistogram2.addS(D1_histogram2);
	return;
}

template < typename T >
void QuickLookNEF<T>::wait() {
	//キー入力が会った時のみループが進む
	//クリックで動作するためにはコールバック関数内に書く必要がある。
	//whild(1){}にすれば全部この中に書けるけど重そう？
	int key = 0;
	bool escFlag = false;

	cout << endl;
	cout << "Key binds  n:Next / b:Back / q:Quit" << endl;

	while((key = waitKey(-1))) {
		//cout << ERROR_LINE << cv::format("%c %d %d", key, key, 'q') << endl;
		//		printf("#%c %d %d\n",key,key,'q');
		switch(key) {
		case 'q' :
			exit(0);
			break;
		case 'i': //initialize 画像処理と表示領域を初期化
			initializeImage();
			setMainPosition(I1.cols / 2, I1.rows / 2, 1);
			update();
			break;
		case 'o': //original 画像処理を初期化
			initializeImage();
			update();
			break;
		case 'r': //re-framing 表示領域を初期化
			setMainPosition(I1.cols / 2, I1.rows / 2, 1);
			update();
			break;
		case 'g':
			gamma_flag = !gamma_flag;
			createNormalizedImage();
			update();
			break;
		case 'd' : //dark area
			dark_mode = true;
			cout << __func__ << " " << endl;
			break;
			//		case 'p' :
			//			imwrite("ql.tif",Ds[iCurrentWindow]);
			//			break;
		case '+' : case ';' :
			if(main_scale == -2) { main_scale = 1; }
			else { main_scale++; }
			update();
			break;
		case '-' :
			if(main_scale == 1) { main_scale = -2; }
			else { main_scale--; }
			update();
			break;
		case ',': case '<' :
			display_sigma -= 1.0;
			max0 = mean0.val[1] + display_sigma * stddev0.val[1];
			createNormalizedImage();
			update();
			break;
		case '.': case '>' :
			display_sigma += 1.0;
			max0 = mean0.val[1] + display_sigma * stddev0.val[1];
			createNormalizedImage();
			update();
			break;
			//		case 'l' :
			//			list();
			//			break;
			//		case KEY_LEFT : case 'b' ://left
			//			prev();
			//			break;
			//		case KEY_RIGHT : case 'n' ://right
			//			next();
			//			break;
		case ' ' : //case 32 :
			//cout << "a" <<
			//			cout << "#" << ERROR_LINE << "Can't get mouse position yet... " ;
			//			cout << iCurrentWindow << " " << nWindow << " " << mx << " " << my << " ";
			//			cout << getImageInfo(Ds[iCurrentWindow]) << endl; break;
			//			printf("%s:(%d,%d) %3d:%3d:%3d\n",names[iCurrentWindow].c_str(),mx,my,
			//					Ds[iCurrentWindow](my,mx)[0],Ds[iCurrentWindow](my,mx)[1],Ds[iCurrentWindow](my,mx)[2]);
			break;
		case KEY_TAB : //Tab
			//			iCurrentWindow = (iCurrentWindow+1 >= nWindow ? 0 : iCurrentWindow+1);
			//			printf("#%s\n",names[iCurrentWindow].c_str());
			break;
		case KEY_ESC : //Esc
			escFlag = true;
			break;
		default :
			printf("#Pressed key %u\n",key);
			break;
		}
		if(escFlag == true) { break; }
	}
}
int main(int argc, char **argv) {
	//オプション解析
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	//NEFを読み込み
	string input = FLAGS_i;

	QuickLookNEF<float> qlnef(input);

	return 0;
}


