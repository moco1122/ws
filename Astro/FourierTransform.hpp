/*
 * FourierTransform.hpp
 *
 *  Created on: 2015/06/04
 *              2018/07/12 : reject boost dependency
 *      Author: Atsushi Kawai
 *
 * OpenCVを使ったFFTの関数
 */
#ifndef FOURIERTRANSFORM_HPP_
#define FOURIERTRANSFORM_HPP_

#include <vector>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

namespace mycv {
//実空間1面入力画像を周波数空間2面複素数画像に変換
Mat DFT(Mat I,int flag = 0,bool square = false);
Mat inverseDFT(Mat I,int flag = 0);
//実空間N面入力画像を周波数空間2面複素数画像xN要素のvectorに変換
vector<Mat> DFTs(Mat I,int flag = 0,bool square = false); //戻り値の型が違うだけではオーバーロードできないので
Mat inverseDFTs(vector<Mat> Fs,int flag = 0); //オーバーロードできるけどDFTsと対応づけて

Mat complexToMag(Mat F);
void swapQuadrant(Mat F);
void swapHorizontal(Mat F);
void swapVertical(Mat F);
vector<Mat> complexToMag(vector<Mat> Fs);
void swapQuadrant(vector<Mat> Fs);
void swapHorizontal(vector<Mat> Fs);
//OpenCVにmulSpectrum()というのがあった...
//Mat multiplyComplex(Mat F,Mat G);
//vector<Mat> multiplyComplex(vector<Mat> Fs,vector<Mat> Gs);

enum WindowFunctionType {NO_WINDOW=0,RECTANGLAR=1,BARTLETT=2,HANN=3,HAMMING=4,
	BLACKMAN=5,BLACKMAN_HARRIS4=6,BLACKMAN_HARRIS7=7};
Mat get2DWindowFunction(WindowFunctionType type,int width,int height,int channels = 1);
Mat applyWindowFunction(Mat I,Mat W,bool zeroCentering);
//Mat getHorizontalWindowFunction(int width);
Mat applyHorizontalWindowFunction(Mat I,Mat W,bool zeroCentering);

//enum AverageType {RADIAL,RADIALHALF,Horizontal,Vertical};
enum AverageType {RADIAL};
vector<float> squaredAverageProfile(Mat_<float> A,AverageType type);
vector<float> getFFT2DAmplitudeProfile(Mat_<float> I);

Mat_<float> convertToPolarImage(Mat_<float> I,int width);
Mat convertToPolarImage(Mat I,int width);
Mat bandpassFiltering(Mat F,float freqMin,float freqMax,float &areaRatio,float freqMinSigma=0.0,float freqMaxSigma=0.0,bool flag=true);
Mat gaussianBandpassFiltering(Mat F,float freq,float sigma,float &areaRatio,bool flag=true);
Mat logRatioGaussianBandpassFiltering(Mat F,float freq,float sigma,float &areaRatio,bool flag=true);

Mat getDFTAmplitude(Mat I,int flag=0,bool square=false);
//Noise.cppで実装
Mat_<float> createWhiteNoise(int width,int height,unsigned long seed,float mean,float sigma);
Mat createAverageImage(Mat I,AverageType type);
Mat convertToPolarForm(Mat O);
Mat convertToOrthogonalForm(Mat P);
vector< Mat > convertToPolarForm(vector< Mat > Os);
vector< Mat > convertToOrthogonalForm(vector< Mat > Ps);

vector< Mat_<float> > createPolarFilters(int w,int h,vector<float> rCenters,float rSigma,
		int nAngles,float dSigma,float freq2cpd,bool nyquistCutoff=true);

} /* namespace mycv */

#endif /* FOURIERTRANSFORM_HPP_ */
