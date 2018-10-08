#include <Rcpp.h>
using namespace Rcpp;
using namespace std;

#include "dcrawFunc.hpp"
//これだけでdcrawFunc.cppをコンパイルしてリンクしてくれる.
//MakevarsでPKG_LIBS=-lm -ljasper -ljpeg -llcms2の指定が必要

//うまくいかないコードの保管場

//OpenCVで表示を試したがうまくいかなかった
// [[Rcpp::export]]
NumericVector timesTwo(NumericVector x) {
  cout << "hello" << endl;
  //  cout << cv::format("%4d",77) << endl;
  //
  //  Mat_<unsigned char> I(256,256);
  //  I = 200;
  //  namedWindow("aaa");
  //  imshow("aaa",I);
  //  waitKey(-1);
  
  return x * 2;
}

//Numericは64bitでサイズが大きいIntegerが32bitなのでそちらを使用
//// [[Rcpp::export]]
// NumericVector readNEF(string fileName) {
//   dcraw::NEF nef = dcraw::readNEF0(fileName);
// //  DataFrame df = DataFrame::create(Named("width")=nef.width, Named("height", nef.height));
//   long n = nef.width*nef.height;
//   NumericVector data00(n);
//   for(int i = 0; i < n; i++) { data00[i] = nef.data0[i]; }
//   free(nef.data0);
//   // df.attr("width") = nef.width;
//   // df.attr("height") = nef.height;
//   return(data00);
// }


//2重ループにすると0.15sほど遅くなった.t()と同じくらいなのでとりあえずあまり気にしないことにする
// [[Rcpp::export]]
IntegerVector readNEF1(string fileName) {
  //そのままbayer配列として読み出すと高速だが、縦横が逆になる。
  
  dcraw::NEF nef = dcraw::readNEF0(fileName);
  //  DataFrame df = DataFrame::create(Named("width")=nef.width, Named("height", nef.height));
  long n = nef.width*nef.height;
  IntegerVector data00(n);
  for(int i = 0; i < n; i++) { data00[i] = nef.data0[i]; }
  free(nef.data0);
  data00.attr("width") = nef.width;
  data00.attr("height") = nef.height;
  return(data00);
}

// [[Rcpp::export]]
IntegerMatrix readNEF2(string fileName) {
  dcraw::NEF nef = dcraw::readNEF0(fileName);
  //  DataFrame df = DataFrame::create(Named("width")=nef.width, Named("height", nef.height));
  int width = nef.width;
  int height = nef.height;
  IntegerMatrix data00(height, width);
  for(int y = 0; y < height; y++) { 
    for(int x = 0; x < width; x++) {
      data00(y, x) = nef.data0[x + y*width];  
    }
  }
  free(nef.data0);
  return(data00);
}


//DataFrameに属性つけて返すと失敗する？
// // [[Rcpp::export]]
// DataFrame readNEF(string fileName) {
//   dcraw::NEF nef = dcraw::readNEF0(fileName);
//   //  DataFrame df = DataFrame::create(Named("width")=nef.width, Named("height", nef.height));
//   long n = nef.width*nef.height;
//   NumericVector data00(n);
//   for(int i = 0; i < n; i++) { data00[i] = nef.data0[i]; }
//   free(nef.data0);
//   
//   //  NumericVector data0(nef.data0);
//   // DataFrame df = DataFrame::create(Named("data00")=data00);
//   // df.attr("width") = nef.width;
//   // df.attr("height") = nef.height;
//   // 
//   // return(df);
//   return DataFrame::create(Named("data00")=data00);
// }

// [[Rcpp::export]]
IntegerVector read_NEF_as_srgb(string fileName) {
  //そのままbayer配列として読み出す
  dcraw::NEF nef = dcraw::readNEF0(fileName);
  int r_x,r_y;
  int gr_x,gr_y;
  int gb_x,gb_y;
  int b_x,b_y;
  r_x  = 0; r_y  = 0;
  gr_x = 1; gr_y = 0;
  gb_x = 0; gb_y = 1;
  b_x  = 1; b_y  = 1;
  
  cout << "a" << nef.cameraName << "b" << endl;
  
  if(nef.cameraName == "\"D90\"") {
    r_x  = 1; r_y  = 0;
    gr_x = 0; gr_y = 0;
    gb_x = 1; gb_y = 1;
    b_x  = 0; b_y  = 1;
  } 
  else {
    Rcerr << "Unknown camera " << nef.cameraName << " " << (nef.cameraName == "\"D90\"") << endl;
  }
  
  int width0 = nef.width;
  int width = nef.width/2;
  int height = nef.height/2;
  long n = width*height;
  
  IntegerVector srgb(n*3);
  
  for(int y = 0; y < height; y++) { 
    for(int x = 0; x < width; x++) {
      //srgb[x + y*width + 0*n] = 0; //0がR
      srgb[x + y*width + 0*n] = nef.data0[(2*x+r_x)  + (2*y+r_y)*width0]; 
      srgb[x + y*width + 1*n] = nef.data0[(2*x+gr_x) + (2*y+gr_y)*width0]; 
      srgb[x + y*width + 2*n] = nef.data0[(2*x+b_x)  + (2*y+b_y)*width0]; 
    } 
  }
  free(nef.data0);
  
  srgb.attr("dim") = IntegerVector::create(width, height, 3); 
  return(srgb);
}



// [[Rcpp::export]]
DataFrame bayerToRGB(DataFrame bayer) {
  unsigned int width = 256;
  unsigned int height = 256;
  unsigned int n = width*height;
  
  NumericVector data0 = bayer[0];
  unsigned int w = bayer.attr("width");
  unsigned int h = bayer.attr("height");
  NumericVector r(n);
  NumericVector g(n);
  NumericVector b(n);
  
#define I(y,x) data0[(y)*w + (x)]
  int tx,ty;
  for(int y = h/4; y < h/4+height; y++) {
    ty = y*2;
    for(int x = w/4; x < w/4 + width; x++) {
      tx = x*2;
      r[(y-h/4)*width + (x-w/4)] = (9*I(ty  ,tx  ) + 3*I(ty  ,tx+2) + 3*I(ty+2,tx  ) + I(ty+2,tx+2))/16;
    }
  }
#undef I
  //	DataFrame rgb = DataFrame::create(Named("R")=r,Named("G")=g,Named("B")=b);
  DataFrame rgb = DataFrame::create(Named("R")=r);
  rgb.attr("width") = width;
  rgb.attr("height") = height;
  
  return(rgb);
}

// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically
// run after the compilation.
//

/*** R
#timesTwo(42)
system.time(b1 <- readNEF1("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF"))
  system.time(b2 <- readNEF2("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3121.NEF"))
#rgb <- bayerToRGB(bayer);
#head(rgb)
  
#as.cimg(rgb$R,x=256,y=256) -> iii
#> window()
# hasTsp(x) でエラー:  引数 "x" がありませんし、省略時既定値もありません
#> quartz()
#> plot(iii)
  */




//参考
//bayerToRGB(unsigned short *data,unsigned int width,unsigned int,float rGain,float bGain) {
//	Mat_<short> R(I.rows/2,I.cols/2);
//	Mat_<short> G(I.rows/2,I.cols/2);
//	Mat_<short> B(I.rows/2,I.cols/2);
//	vector< Mat_<short> > Ov;
//
////	 0123456789
////	0RGRGRGRGRG
////	1GBGBGBGBGB
////	2RGRGRGRGRG
////	3GBGBGBGBGB
////	4RGRGRGRGRG
////	5GBGBGBGBGB
//
//	int tx,ty;
//	for(int y = 1; y < R.rows-1; y++) {
//		for(int x = 1; x < R.cols-1; x++) {
//			tx = 2*x;
//			ty = 2*y;
//			B(y,x) = (9*I(ty  ,tx  ) + 3*I(ty  ,tx+2) + 3*I(ty+2,tx  ) + I(ty+2,tx+2))/16;
//			G(y,x) = (9*I(ty  ,tx+1) + 3*I(ty  ,tx-1) + 3*I(ty+2,tx+1) + I(ty+2,tx-1) +
//					  9*I(ty+1,tx  ) + 3*I(ty+1,tx+2) + 3*I(ty-1,tx  ) + I(ty-1,tx+2))/32;
//			R(y,x) = (9*I(ty+1,tx+1) + 3*I(ty+1,tx-1) + 3*I(ty-1,tx+1) + I(ty-1,tx-1))/16;
//		}
//	}
//
//	R = rGain*R;
//	B = bGain*B;
//
//	Ov.push_back(R);
//	Ov.push_back(G);
//	Ov.push_back(B);
//
//	Mat_<Vec3s> O;
//
//	cv::merge(Ov,O);
//
//	return(O);
//}
//
//int main(int argc, const char**argv) {
//	cv::Mat I;
//	//	printf("hello\n");
//	cout << "hello lib " << endl;
//	//	dcraw::tprint();
//	dcraw::NEF nef = dcraw::readNEF(argv[1]);
////	int targc = 4;
////	const char **targv = (const char **)calloc(targc,sizeof(char *));
////	targv[0] = argv[0];
////	targv[1] = "-T";
////	targv[2] = "-E";
////	targv[3] = argv[1];
////	dcraw::NEF nef = dcraw::readNEF(targc,targv);
//	int offset = atoi(argv[2]);
//	float rGain = atof(argv[3]);
//	float bGain = atof(argv[4]);
//	float gain = atof(argv[5]);
//
////	cout << "Write image " << nef.status << " " << nef.I0.cols << "x" << nef.I0.rows << endl;
////	cv::imwrite("I0b.tif",nef.I0);
////
////	namedWindow("I",CV_WINDOW_AUTOSIZE);
////	imshow("I",nef.I0);
//	Mat_<unsigned short> I00(nef.height,nef.width,nef.data0);
//
//	Mat_<short> I0 = I00-offset;
//
//	Mat_<Vec3s> O = bayerToRGB(I0,rGain,bGain);
//
//	cout << O.cols << " " << O.rows << " " << O.channels() << " " << O.type() << endl;
////
//	resize(O,O,Size(),0.25,0.25);
//
//	int maxValue = (1<<14)-1;
////	O = O - 300;
//	O = O * gain;
//	//O = (O-1000) * 4;
//	Mat_<float> fO = O / maxValue;
//	cv::pow(fO,1/2.2,fO);
//	fO = fO * 255;
//	Mat_<Vec3b> D0 = fO;
//	Mat_<Vec3b> D;
//	transpose(D0,D);
//	flip(D,D,0);
//	namedWindow("O",CV_WINDOW_AUTOSIZE);
//	imshow("O",D);
//
//	waitKey(0);
//
////	nef.printInfoHeader();
////	nef.printInfo();
//
//	return(0);
//}
