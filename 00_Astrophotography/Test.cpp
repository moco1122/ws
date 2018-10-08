/*
 * Test.cpp
 *
 *  Created on: 2015/10/08
 *      Author: kawai
 */

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

int main(int argc, char **argv) {
	using namespace std;
	using namespace cv;

	Mat_<short> I(256,256,77);


	cout << "hello" << endl;

	cout << I << endl;

	  cv::namedWindow("image1", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	  // ウィンドウ名でウィンドウを指定して，そこに画像を描画
	  cv::imshow("image1", I);

	  // デフォルトのプロパティで表示
	  cv::imshow("image2", I);

	  // キー入力を（無限に）待つ
	  cv::waitKey(0);
}



