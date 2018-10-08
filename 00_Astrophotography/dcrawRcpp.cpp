#include <Rcpp.h>
#include <string.h>

using namespace Rcpp;
using namespace std;

#include "dcrawFunc.hpp"
//これだけでdcrawFunc.cppをコンパイルしてリンクしてくれる.
//MakevarsでPKG_LIBS=-lm -ljasper -ljpeg -llcms2の指定が必要

// // [[Rcpp::plugins("cpp11")]]
// [[Rcpp::export]]
IntegerVector read_NEF_as_bayer(string fileName) {
  //そのままbayer配列として読み出す
  dcraw::NEF nef = dcraw::readNEF0(fileName);
  int width = nef.width;
  int height = nef.height;
  long n = width*height;
  
  IntegerVector bayer(n);
  
  for(int y = 0; y < height; y++) { 
    for(int x = 0; x < width; x++) {
      //      bayer[y + x*height] = nef.data0[x + y*width];  //Rのarrayと縦横が逆
      bayer[x + y*width] = nef.data0[x + y*width];  //Rのarrayと縦横が逆
    } 
  }
  free(nef.data0); 
  
  
  
  
  bayer.attr("dim") = IntegerVector::create(width, height, 1); 
  return(bayer);
}

// [[Rcpp::export]]
IntegerVector read_NEF_as_srgb(string fileName, bool align_barycenter = true) {
  //そのままbayer配列として読み出す
  dcraw::NEF nef = dcraw::readNEF0(fileName);
  int x1,y1; //bayerでRとなる最初の画素
  x1 = 0; y1 = 0;
  if(nef.cameraName == "\"D90\"") { //GrRBGb
    x1  = 1; y1  = 0;
  }
  else {
    Rcerr << "Unknown camera " << nef.cameraName << " " << (nef.cameraName == "\"D90\"") << endl;
  }
  
  int w0,h0,w,h;
  w0 = nef.width; //データ上のサイズ
  h0 = nef.height; //データ上のサイズ
  w = (w0 - x1) / 2; w = w * 2; //R開始/B終了にした時のサイズ
  h = (h0 - y1) / 2; h = h * 2; //R開始/B終了にした時のサイズ
  
//  cout << "a" << nef.cameraName << "b " << align_barycenter << endl;
//  Rcout << "align_barycenter == true" << endl;
  
  int width = w / 2;
  int height = h / 2;
  long npix = width*height;
  
  IntegerVector srgb(npix*3);
  
  unsigned short *d0 = nef.data0;
  const int kR = 0;
  const int kG = 1;
  const int kB = 2;
#define P(tx, ty) d0[(tx) + (ty)*w0]
//#define S(tm, tn, tc) srgb[(tm) + (tn)*width + (tc)*npix]
#define S(tm, tn, tc) srgb[(tm)*height + (tn) + (tc)*npix]
  if(align_barycenter != true) {
    for(int y=y1, n=0; y < y1+h-1; y=y+2, n++) { 
      for(int x=x1, m=0; x < x1+w-1; x=x+2, m++) {
        //重心不一致
        // srgb[m + n*width + 0*npix] = nef.data0[x   + (y  )*w0]; //R
        // srgb[m + n*width + 1*npix] = nef.data0[x+1 + (y  )*w0]; //G
        // srgb[m + n*width + 2*npix] = nef.data0[x+1 + (y+1)*w0]; //B
        S(m, n, kR) = P(x  , y  );
        S(m, n, kG) = (P(x+1, y  ) + P(x  , y+1)) / 2;
        S(m, n, kB) = P(x+1, y+1);
      } 
    }
  }
  else {
    for(int y=y1, n=0; y < y1+h-2; y=y+2, n++) { 
      for(int x=x1, m=0; x < x1+w-2; x=x+2, m++) {
        //重心一致
        //RGRG
        //GBGB
        //RGRG
        //GBGB
        S(m, n, kR) = (9*P(x+2, y+2) + 3*P(x+2, y  ) + 3*P(x  , y+2) + P(x  , y  )) / 16;
        S(m, n, kG) = (9*P(x+1, y+2) + 3*P(x+1, y  ) + 3*P(x+3, y+2) + P(x+3, y  ) 
                         + 9*P(x+2, y+1) + 3*P(x+2, y+3) + 3*P(x  , y+1) + P(x  , y+3) ) / 32;
        S(m, n, kB) = (9*P(x+1, y+1) + 3*P(x+1, y+3) + 3*P(x+3, y+1) + P(x+3, y+3)) / 16;;
      } 
    }
    
  }
  //free(nef.data0);
#undef P
#undef S
//  srgb.attr("dim") = IntegerVector::create(width, height, 3); 
  srgb.attr("dim") = IntegerVector::create(height, width, 3); 
  return(srgb);
}

// dimをしていすればarrayに　y->x->z
// read_NEF_as_srgb
// read_NEF_as_bayer
// read_NEF_as_rggb
//   matrixとvector+dim+t()で実装して時間比較
//system.time(b1 <- read_NEF_as_bayer("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF"))
//system.time(b2 <- readNEF2("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3121.NEF"))

