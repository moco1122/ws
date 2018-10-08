/*
 * QuickLook.cpp
 *
 *  Created on: 2013/11/15
 *      Author: Atsushi Kawai
 *
 * 画像表示確認用のクラス
 */
#include <opencv2/opencv.hpp>

#include "Utils++.hpp"
#include "QuickLook.hpp"

using namespace cv;
using namespace std;

namespace mycv {

QuickLook::QuickLook() {
	initialize(ARRANGE,"noname");
}

QuickLook::QuickLook(Mode mode,string windowName) {
	initialize(mode,windowName);
}
void QuickLook::initialize(Mode mode,string windowName,int wx,int wy) {
	this->mode = mode;
	this->windowName = windowName;
//	//Windows7
//	wxMargin = 16;
//	wyMargin = 38;
//	wx0 = 16;
//	wy0 = 16;
	//Mac OSX
	wxMargin = 2;
	wyMargin = 2;
	wx0 = 0;
	wy0 = 24;
	this->wx = wx + wx0;
	this->wy = wy + wy0;
	wxn = 0;
	wyn = 0;

	iCurrentWindow = -1;
	nWindow = 0;

	mx = -1;
	my = -1;

	verbose = false;
}
void QuickLook::initializeArrangeMode(string windowName,int wx,int wy) {
	initialize(ARRANGE,windowName,wx,wy);
}
void QuickLook::initializeStackMode(string windowName,int wx,int wy) {
	initialize(STACK,windowName,wx,wy);

}

QuickLook::~QuickLook() {

}

void cbMousePrintWindowName(int event,int x,int y,int flag,void *data) {
	QuickLook *ql = reinterpret_cast<QuickLook *>(data);
	//	string message;
	//cout << flag << endl;
	switch(event) {
	case EVENT_MOUSEMOVE:
		ql->mx = x;
		ql->my = y;
	//		message += "MOUSE_MOVE ";
		break;
	case EVENT_LBUTTONDOWN :
		printf("lbutton %d %s\n",ql->wxn,ql->names[0].c_str()); fflush(stdout);
		break;
	default: return; break;
	}
	//	string *windowName = reinterpret_cast<string *>(data);
	//	printf("%s:%s (%d,%d)\n",windowName->c_str(),message.c_str(),x,y); fflush(stdout);
	return;
}

Mat_<Vec3b> QuickLook::convert(Mat M) {
	Mat tM;
	Mat_<Vec3b> tD;
	if(M.type() != CV_8U || M.type() != CV_8UC1 || M.type() != CV_8UC3 ||
			M.type() != CV_16U || M.type() != CV_16UC1 || M.type() != CV_16UC3 ||
			M.type() != CV_32F || M.type() != CV_32FC1 || M.type() != CV_32FC3) {
		if(M.channels() == 1) {
			M.convertTo(tM,CV_8U);// 1.0->255になるので?
		}
		else if(M.channels() == 3) {
			M.convertTo(tM,CV_8UC3);
		}
	}
	if(tM.channels() != 3) {
		cvtColor(tM,tD,CV_GRAY2BGR);
	}
	else { tD = tM.clone(); }
	return(tD);
}

void QuickLook::add(string name, Mat M, int x, int y, int scale, int waitTime) {
	names.push_back(name);

	Ms.push_back(M);

	Mat_<Vec3b> tD = convert(M);
	Mat_<Vec3b> D;
	double factor;
	if(scale == 1) { D = tD; }
	else {
		if(scale >= 0) { factor = scale; }
		else { factor = 1.0 / abs(scale); }
		resize(tD, D, Size(), factor, factor, ::INTER_NEAREST);
	}

	Ds.push_back(D);
	wps.push_back(Point(x,y));

	//	printf("%d %d\n",x,y);
//	if()
//	CV_Assert(x > 0 && y > 0);
	wx = x;
	wy = y;

	//cout << wx0 << " " << wy0 << " " << wx << " " << wy << endl;
	if(mode == ARRANGE) {
		imshow(name,D);
		cvMoveWindow(name.c_str(),x,y);
		wxn = D.cols + wxMargin;
		wyn = D.rows + wyMargin;
	}
	else if(mode == STACK) {
		imshow(windowName,D);
		cvMoveWindow(windowName.c_str(),x,y);
	}

	nWindow++;
	iCurrentWindow = nWindow-1;

	//setMouseCallback(name,cbMousePrintWindowName,this);

	if(waitTime != 0) { //cv::waitKeyは0でも無限待ちになるので
		waitKey(waitTime);
	}
}

void QuickLook::add(string name,Mat M,Direction d, int scale,int waitTime) {
//	std::cout << name << " " << wx << " " << wy << " " << endl;
	int x = 0;
	int y = 0;
	switch(d) {
	case H :
		x = wx + wxn;
		y = wy;
		break;
	case V :
		x = wx;
		y = wy + wyn;
		break;
	}
	add(name, M, x, y, scale, waitTime);
}
void QuickLook::addH(string name, Mat M, int scale, int waitTime) {
	add(name, M, H, scale, waitTime);
}
void QuickLook::addV(string name, Mat M, int scale, int waitTime) {
	add(name, M, V, scale, waitTime);
}
void QuickLook::addS(string name, Mat M, int scale, int waitTime) {
	mode = STACK;
	add(name, M, wx, wy, scale, waitTime);
}
void QuickLook::addS(Mat M, int scale, string description, int waitTime) {
	mode = STACK;
	add(windowName, M, wx, wy, scale, waitTime);
	if(description.empty() != true) {
		TextWriter tw;
		tw.describe(Ds.back(), description, CV_GREEN, CV_BLACK1);
		imshow(windowName, Ds.back());
	}
}
void QuickLook::setPosition(int wx,int wy) {
	this->wx = wx;
	this->wy = wy;
//	if(mode == STACK) {
		wxn = 0;
		wyn = 0;
//	}
}
void QuickLook::setPositionNext() {
	setPosition(wx+wxn,wy);
}

void QuickLook::setPositionNextH() {
	setPosition(wx0, wy + wyn);
}

void QuickLook::setPositionNextV() {
	setPosition(wx + wxn, wy0);
}
void QuickLook::wait() {
	int key = 0;
	bool escFlag = false;

	cout << endl;
	cout << "Key binds  n:Next / b:Back / q:Quit" << endl;

	while((key=waitKey(-1))) {
//				printf("%c %d %d\n",key,key,'q');
//		printf("#%c %d %d\n",key,key,'q');
		switch(key) {
		case 't' :
			return;
			break;
		case 'p' :
			imwrite("ql.tif",Ds[iCurrentWindow]);
			break;
		case 'q' :
			exit(0);
			break;
		case 'l' :
			list();
			break;
		case KEY_LEFT : case 'b' ://left
			prev();
			break;
		case KEY_RIGHT : case 'n' ://right
			next();
			break;
		case ' ' : //case 32 :
			cout << "#" << ERROR_LINE << "Can't get mouse position yet... " ;
			cout << iCurrentWindow << " " << nWindow << " " << mx << " " << my << " ";
			cout << getImageInfo(Ds[iCurrentWindow]) << endl; break;
			printf("%s:(%d,%d) %3d:%3d:%3d\n",names[iCurrentWindow].c_str(),mx,my,
					Ds[iCurrentWindow](my,mx)[0],Ds[iCurrentWindow](my,mx)[1],Ds[iCurrentWindow](my,mx)[2]);
			break;
		case KEY_TAB : //Tab
			iCurrentWindow = (iCurrentWindow+1 >= nWindow ? 0 : iCurrentWindow+1);
			printf("#%s\n",names[iCurrentWindow].c_str());
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
void QuickLook::list() {
	int size = (int)names.size();
	for(int i = 0; i < size; i++) {
		printf("%d/%d %s\n",i,size,names[i].c_str());
	}
}
void QuickLook::setImage(string windowName,int i) {
	int size = (int)names.size();
	if(verbose == true) {
		procMessage("%d/%d %s\n",i,size,names[i].c_str());
	}
	imshow(windowName,Ds[i]);
}
void QuickLook::next() {
//	cout << ERROR_LINE << "Not implemented yet." << endl;
	if(mode != STACK) { return; }
//	cout << ERROR_LINE << "Not implemented yet. " << iCurrentWindow << " " << nWindow << endl;
	CV_Assert(mode == STACK);
	iCurrentWindow = (iCurrentWindow+1 >= nWindow ? 0 : iCurrentWindow+1);
	setImage(windowName,iCurrentWindow);
}
void QuickLook::prev() {
	if(mode != STACK) { return; }
	CV_Assert(mode == STACK);
	iCurrentWindow = (iCurrentWindow-1 < 0 ? nWindow-1 : iCurrentWindow-1);
	setImage(windowName,iCurrentWindow);
}

} // namespace mycv
