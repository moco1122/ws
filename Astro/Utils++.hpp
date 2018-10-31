/*
 * Utils++.hpp
 *
 *  Matを使わないユーティリティ関数類
 */
#ifndef UTILSPP_HPP_
#define UTILSPP_HPP_
#include <string>
//#define VS2012

//C++17からfilesystemが入るらしいが、まだ使えない場合が多そうなので保留 20180710
//#ifdef VS2012
//#else
//#include <experimental/filesystem>
//namespace fs = std::experimental::filesystem ;
//#endif

//古いエラー出力用マクロ。
//#include <stdio.h>
#include <stdlib.h>
#define FMT_FUNC "#%-25.25s : "
#define FMT_FUNC_LINE "#%s : %4d : "
#define errorMessage(fmt,...) fprintf(stderr,FMT_FUNC_LINE fmt,__func__,__LINE__,__VA_ARGS__); fflush(stderr);
#define errorMessage0(message) fprintf(stderr,FMT_FUNC_LINE "%s",__func__,__LINE__,message); fflush(stderr);
#define errorMessageExit(fmt,...) fprintf(stderr,FMT_FUNC_LINE fmt,__func__,__LINE__,__VA_ARGS__); exit(-1);
#define errorMessage0Exit(message) fprintf(stderr,FMT_FUNC_LINE "%s",__func__,__LINE__,message); exit(-1);
#define procMessage(fmt,...)  printf("#%-25.25s : " fmt,__func__,__VA_ARGS__); fflush(stdout);
#define procMessage0(message)  fprintf(stdout,FMT_FUNC "%s",__func__,message); fflush(stdout);
#define procMessageFunc0(func,message)  fprintf(stdout,FMT_FUNC "%s",func,message); fflush(stdout);
#define procMessageFunc(func,fmt,...)  fprintf(stdout,FMT_FUNC fmt,func,__VA_ARGS__); fflush(stdout);

#include <string>
#include <vector>

namespace mycv {
using namespace std;

//エラー出力用マクロ
//cout << ERROR_LINE << "Not implemented yet." << endl;
//cerr << ERROR_LINE << "error" << endl; exit(-1);
#define ERROR_LINE cv::format("[%5d]%s:%s(): ",__LINE__,__FILE__,__func__)


//コンパイル環境の型サイズをチェックするための関数
void checkSizeOf();
void printSizeOf();

//プログラムの引数処理用の関数 Utils++.cppの最後にインスタンス化。
vector<string> getImageFilesList(const string path);
bool isExist(const string& filename);
string replaceAll(string &str, string from, string to);
void prepareDirectory(string dirname);
string getLeafname(const string fileName);
string getExtension(const string fileName);
string getBasename(const string fileName);
template <class T>
vector<T> parseCoefficients(string kernelArg);
template <class T>
string vectorToString(vector<T> v);
char **parseArgsFile(int* argc,char **argv);

string getLatestNEFFile(const string dir, int last);

//VisualStudioにない関数をどうにかする
#ifdef VS2012
#pragma warning(disable:4819)
#define __func__ __FUNCTION__
void setStartTime();
int round(double d);
#endif

//計算資源管理用の関数
string getTimeInfo();
string getUsageInfo();
string getUsageInfoKB();
#define PRINTUSAGEINFO (std::cout << ERROR_LINE << " " << getUsageInfo() << endl)
#define PRINTUSAGEINFOKB (std::cout << ERROR_LINE << " " << getUsageInfoKB() << endl)

} //end of namespace mycv

//添え字によるソートのための比較関数を提供する構造体
//mycvやUtils++.cppに入れない理由が何かあったような気がする...
template < class T >
struct compareForIndexSort{
	bool operator()(const int& a, const int& b) const {
		return v[a]<v[b];
	}
	compareForIndexSort(const T *p):v(p){}
private:
	const T *v;
};
template struct compareForIndexSort<short>;
template struct compareForIndexSort<float>;
template struct compareForIndexSort<double>;

#endif

