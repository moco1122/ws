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
	  // �E�B���h�E���ŃE�B���h�E���w�肵�āC�����ɉ摜��`��
	  cv::imshow("image1", I);

	  // �f�t�H���g�̃v���p�e�B�ŕ\��
	  cv::imshow("image2", I);

	  // �L�[���͂��i�����Ɂj�҂�
	  cv::waitKey(0);
}



