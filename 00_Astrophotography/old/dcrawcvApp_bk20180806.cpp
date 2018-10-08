#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include "dcrawFunc.cpp"

using namespace std;
using namespace cv;

DEFINE_string(i, "unknown.nef", "Input NEF file.");
DEFINE_string(f, "unknown", "flat frame.");
DEFINE_double(gain, 1, "Gain");
DEFINE_string(WB, "1,1", "White balance rGain,bGain");
DEFINE_string(bg, "", "file name to write or read");
DEFINE_string(F, "unknown_flag", "Create falt frame");
DEFINE_int32(s, 0, "Smoothing sigma");
DEFINE_int32(S, 1, "Scaling factor");
DEFINE_int32(R, 0, "Rotation 0:none/1:+90/2:+180/3+270");
//
Mat_<Vec3s> bayerToRGB(Mat_<unsigned short> I,float rGain,float bGain) {
	cout << "#" << __func__ << endl;

	Mat_<short> R(I.rows/2,I.cols/2);
	Mat_<short> G(I.rows/2,I.cols/2);
	Mat_<short> B(I.rows/2,I.cols/2);
	vector< Mat_<short> > Ov;

	//	 0123456789
	//	0RGRGRGRGRG
	//	1GBGBGBGBGB
	//	2RGRGRGRGRG
	//	3GBGBGBGBGB
	//	4RGRGRGRGRG
	//	5GBGBGBGBGB

	int tx,ty;
	for(int y = 1; y < R.rows-1; y++) {
		for(int x = 1; x < R.cols-1; x++) {
			tx = 2*x;
			ty = 2*y;
			R(y,x) = (9*I(ty  ,tx  ) + 3*I(ty  ,tx+2) + 3*I(ty+2,tx  ) + I(ty+2,tx+2))/16;
			G(y,x) = (9*I(ty  ,tx+1) + 3*I(ty  ,tx-1) + 3*I(ty+2,tx+1) + I(ty+2,tx-1) +
					9*I(ty+1,tx  ) + 3*I(ty+1,tx+2) + 3*I(ty-1,tx  ) + I(ty-1,tx+2))/32;
			B(y,x) = (9*I(ty+1,tx+1) + 3*I(ty+1,tx-1) + 3*I(ty-1,tx+1) + I(ty-1,tx-1))/16;
		}
	}

	short maxValue = (1 << 15) - 1;

//	Scalar mean,stddev;
//	cv::meanStdDev(R,mean,stddev);
//	cout << __LINE__ << " " << mean << " " << stddev << endl;
//	cout << maxValue << endl;

	R = R * rGain;
	B = B * bGain;
	R = cv::min(R, maxValue);
	B = cv::min(B, maxValue);

//	cv::meanStdDev(R,mean,stddev);
//	cout << __LINE__ << " " << mean << " " << stddev << endl;

	Ov.push_back(B);
	Ov.push_back(G);
	Ov.push_back(R);

	Mat_<Vec3s> O;

	cv::merge(Ov,O);

	return(O);
}

// -F フラット作成モード 画像中央1/4の平均値で正規化して(1<<14)-1倍し、unsigned shortで保存
// -i 通常モード

int main(int argc, char**argv) {
	//gflagsで定義したフラグを取り除く
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	string input,inputFlat;
	string output;
	string dark = "nodark";
	string flat = "noflat";
	int offset = 600; //カメラ依存
	float gain = 1.0;
	float rGain=2.0, bGain=2.0; //グレーチャートで実測
	Vec3f background = Vec3f(0.0,0.0,0.0);
	float gamma = 1.0;
	float sigma = 3.0;
	float scale = 1.0;
	int rotation = 0;

	enum Mode {OBJECT=0, FLAT=1} mode = OBJECT;

	bool darkCorrection = false;
	bool flatFieldCorrection = false;
	bool whiteBalanceCorrection = false;
	bool backGroundSubtraction = false;

	input = FLAGS_i;
	flat = FLAGS_f;
	gain = FLAGS_gain;
	if(!FLAGS_WB.empty()){
		stringstream ss(FLAGS_WB);
		string s;
		getline(ss, s, ','); rGain = atof(s.c_str());
		getline(ss, s, ','); bGain = atof(s.c_str());
	}
	if(!FLAGS_bg.empty()){
		stringstream ss(FLAGS_bg);
		string s;
		float r,g,b;
		getline(ss, s, ','); r = atof(s.c_str());
		getline(ss, s, ','); g = atof(s.c_str());
		getline(ss, s, ','); b = atof(s.c_str());
		background = Vec3f(b,g,r);
	}
	if(!FLAGS_F.empty()) {
		mode = FLAT;
	}

//	sigma = FLAGS_s;
//	scale = FLAGS_S;
//	R = FLAGS_R;

	cout << input << " " << flat << " " << sigma << " " << mode << " " << rGain << " " << bGain << endl;

	cv::Mat I;

	dcraw::NEF nef = dcraw::readNEF0(input);
	//	int targc = 4;
	//	const char **targv = (const char **)calloc(targc,sizeof(char *));
	//	targv[0] = argv[0];
	//	targv[1] = "-T";
	//	targv[2] = "-E";
	//	targv[3] = argv[1];
	//	dcraw::NEF nef = dcraw::readNEF(targc,targv);

	//	cout << "Write image " << nef.status << " " << nef.I0.cols << "x" << nef.I0.rows << endl;
	//	cv::imwrite("I0b.tif",nef.I0);
	//
	//	namedWindow("I",CV_WINDOW_AUTOSIZE);
	//	imshow("I",nef.I0);
	Mat_<unsigned short> I00(nef.height,nef.width,nef.data0); //unsigned 14bit

	Mat_<short> I0 = I00 - offset; //signed 14bit
//	Scalar mean,stddev;
//	cv::meanStdDev(I0,mean,stddev);
//	cout << mean << " " << stddev << endl;

	Mat_<Vec3s> O = bayerToRGB(I0, rGain, bGain); //signed 15bit

//	cout << O.cols << " " << O.rows << " " << O.channels() << " " << O.type() << endl;

	Scalar mean,stddev;
	double minVal,maxVal;
	if(mode == FLAT) {
		cv::meanStdDev(O,mean,stddev);
		cout << mean << " " << stddev << endl;

		cv::meanStdDev(O(Range(O.rows/4,O.rows*3/4),Range(O.cols/4,O.cols*3/4)),mean,stddev);
		cout << mean << " " << stddev << endl;
		vector< Mat_<float> > vO;
		split(O,vO);
		for(int b = 0; b <= 2; b++) {
			cv::meanStdDev(vO[b](Range(vO[b].rows/4,vO[b].rows*3/4),Range(vO[b].cols/4,vO[b].cols*3/4)),mean,stddev);
//			cout << mean << " " << stddev << endl;
			vO[b] = vO[b] / mean.val[0];
			cv::meanStdDev(vO[b],mean,stddev);
			cout << mean << " " << stddev << endl;
		}
		Mat fO;
		merge(vO,fO);
		cv::meanStdDev(fO(Range(fO.rows/4,fO.rows*3/4),Range(fO.cols/4,fO.cols*3/4)),mean,stddev);
		cout << mean << " " << stddev << endl;

		minMaxLoc(fO,&minVal,&maxVal);
		cout << __LINE__ << " " << minVal << " " << maxVal << endl;
		if(minVal < 0.0) { cerr << __LINE__ << " " << minVal << endl; }
		if(maxVal >= 2.0) { cerr << __LINE__ << " " << maxVal << endl; }

		fO = fO * ((1<<14) - 1);

		Mat_<Vec3w> tO = fO;
		imwrite("flat.tif",tO);
		cv::meanStdDev(tO(Range(tO.rows/4,tO.rows*3/4),Range(tO.cols/4,tO.cols*3/4)),mean,stddev);
		cout << __LINE__ << " " << mean << " " << stddev << endl;
		O = tO;
	}

	int maxValue = (1<<14)-1;
	vector< Mat > vO;
	split(O,vO);
	vector< Mat_<float> > vfF(3);

	if(flat != "noflat") {
		Mat_<Vec3w> F = imread(flat.c_str(),-1);
		cv::meanStdDev(F(Range(F.rows/4,F.rows*3/4),Range(F.cols/4,F.cols*3/4)),mean,stddev);
		cout << __LINE__ << " " << mean << " " << stddev << endl;

		minMaxLoc(F,&minVal,&maxVal);
		cout << __LINE__ << " " << minVal << " " << maxVal << endl;

		vector< Mat_<unsigned short> > vF;
		split(F,vF);
		for(int b = 0; b <= 2; b++) {
			vfF[b] = vF[b] / ((1<<14) - 1);
		}
	}

	for(int b = 0; b <= 2; b++) {
		Mat_<float> fO = Mat_<float>(vO[b]);

		if(flat != "noflat") {
//			cout << fO.cols << " " << fO.rows << " " << fF.cols << " " << fF.rows << endl;
//			cout << fO.channels() << " " << fF.channels() << endl;
			fO = fO / vfF[b];
		}

		Mat_<float> rfO;
		resize(fO,rfO,Size(),0.25,0.25);
		minMaxLoc(rfO,&minVal,&maxVal);
		cout << b << " " << minVal << " " << maxVal << endl;

		if(background.val[b] != 0.0) {
			fO = fO - background.val[b];
		}

		if(gain != 1.0) {
			fO = fO * gain;
		}

//bg,回転実装　順番見直し
//複数枚合成
		fO = fO / maxValue;

		if(gamma != 1.0) {
			cv::pow(fO,1/gamma,fO);
		}

		fO = fO * 255;
		vO[b] = Mat_<short>(fO);
	}

	merge(vO,O);

	if(scale != 1) {
		cv::resize(O,O,Size(),scale,scale);
	}

	Mat_<Vec3b> D0 = O;
	Mat_<Vec3b> D;
	D = D0;
	//回転はビューアで実装
//	if(rotation == 0) { D = D0; }
//	else if(rotaion == 1) {
//		transpose(D0,D);
//		flip(D,D,0);
//	}
	namedWindow("O",CV_WINDOW_AUTOSIZE);
	imshow("O",D);

	waitKey(0);

	//	nef.printInfoHeader();
	//	nef.printInfo();

	return(0);
}



