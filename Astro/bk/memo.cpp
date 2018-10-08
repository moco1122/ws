/*
 * memo.cpp
 *
 */


//QuiclLookにパラメータを設定

DEFINE_string(wargs, "", "wx0,wy0,wx,wy,wxmargin,wymargin");

int main(int argc, char **argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	cout <<  "hello" << std::endl;

	Mat3b D(256, 256);

	QuickLook ql;

	if(FLAGS_wargs.empty() != true) {
		vector< int > wargs = parseCoefficients<int>(FLAGS_wargs);
		ql.wx0 = wargs[0];
		ql.wy0 = wargs[1];
		ql.wx  = wargs[2] + ql.wx0;
		ql.wy  = wargs[3] + ql.wy0;
		ql.wxMargin = wargs[4];
		ql.wyMargin = wargs[5];
	}
	ql.addH("a1", D);
	ql.addH("a2", D);
	ql.addH("a3", D);
	ql.wait();

	//処理時間比較
	start_time();
	dcraw::NEF nef = dcraw::readNEF0(input);
	cout << elapsed_time() << "s" << endl;

	cout << nef.getEXIFHeader() << endl;
	cout << nef.getEXIFInfo() << endl;
	cout << getImageInfo(nef.bayer) << endl;

	start_time();
	Mat_<Vec3d> dS = bayerToRGB<double>(nef.bayer, offset, gain, rGain, bGain);
	cout << elapsed_time() << "s" << endl;
	start_time();
	Mat_<Vec3s> sS = bayerToRGB<short>(nef.bayer, offset, gain, rGain, bGain);
	cout << elapsed_time() << "s" << endl; //shortの方が3倍高速
	start_time();
	Mat_<Vec3f> fS = bayerToRGB<float>(nef.bayer, offset, gain, rGain, bGain);
	cout << elapsed_time() << "s" << endl; //floatの方が2倍高速

	return 0;
}
