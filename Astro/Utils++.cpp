/*
 * Utils++.cpp
 *
 *  Matを使わないユーティリティ関数類
 */
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#ifdef VS2012
#include <windows.h>
#include <stdexcept>
#include <atlstr.h>

#include <time.h>     // for clock()
#include <Psapi.h>
namespace mycv {
clock_t startTime;
void setStartTime() { startTime = clock(); }
int round(double d) { return static_cast<int>(d + 0.5); }
}
#else
#include <dirent.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <direct.h>
#endif

#ifdef USE_BOOST
//OSXでは重くないのでOK
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace fs = boost::filesystem;
using boost::posix_time::ptime;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
//using std::experimental::posix_time::ptime;
using ptime = fs::file_time_type;
#endif

using namespace std;
namespace chrono = std::chrono;

//cv::format()のためにインクルードが必要
#include <opencv2/opencv.hpp>
#include "Utils++.hpp"

namespace mycv {

//型のサイズが想定と異なっていたら終了する。
void checkSizeOf() {
	//64bitコンパイラに変更したときに変わったようなのでlong,unsigned longのサイズを8に変更
	if(sizeof(char) != 1 || sizeof(unsigned char) != 1 || sizeof(short) != 2 || sizeof(unsigned short) != 2 ||
			sizeof(int) != 4 || sizeof(unsigned int) != 4 ||
			sizeof(long) != 8 ||  sizeof(unsigned long) != 8 ||  sizeof(float) != 4 ||
			sizeof(long long) != 8 ||  sizeof(unsigned long long) != 8 ||  sizeof(double) != 8 ) {
		printf("different sizeof() value/assumed value\n");
		printf("char %lu/%d\n",sizeof(char),1);
		printf("unsigned char %lu/%d\n",sizeof(unsigned char),1);
		printf("short %lu/%d\n",sizeof(short),2);
		printf("unsinged short %lu/%d\n",sizeof(unsigned short),2);
		printf("int %lu/%d\n",sizeof(int),4);
		printf("unsigned int %lu/%d\n",sizeof(unsigned int),4);
		printf("long %lu/%d\n",sizeof(long),4);
		printf("unsigned long %lu/%d\n",sizeof(unsigned long),4);
		printf("long long %lu/%d\n",sizeof(long long),8);
		printf("unsigned long long %lu/%d\n",sizeof(unsigned long long),8);
		printf("float %lu/%d\n",sizeof(float),4);
		printf("double %lu/%d\n",sizeof(double),8);

		exit(0);
	}
}
//型のサイズを表示する。
void printSizeOf() {
	printf("char %lubytes\n",sizeof(char));
	printf("short %lubytes\n",sizeof(short));
	printf("int %lubytes\n",sizeof(int));
	printf("long %lubytes\n",sizeof(long));
	printf("long long %lubytes\n",sizeof(long long));
	printf("float %lubytes\n",sizeof(float));
	printf("double %lubytes\n",sizeof(double));
	printf("unsinged short %lubytes\n",sizeof(unsigned short));
}

#ifdef VS2012
vector<string> readImageFiles(string path, string extension) {
	vector<string> image_files;
	//連番画像ファイル名を読込
    HANDLE hFind;
    WIN32_FIND_DATA win32fd;//defined at Windwos.h

    //拡張子の設定
    string search_name = path + "\\*." + extension;

	hFind = FindFirstFile( search_name.c_str() , &win32fd);

    if (hFind == INVALID_HANDLE_VALUE) {
		return(image_files);
		//cerr << " file not found " << search_name << endl;
  //      throw std::runtime_error("file not found");
    }

    do {
        if ( win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        }
        else {
 			image_files.push_back(path + "/" + win32fd.cFileName);
            //printf("%s\n", image_files.back().c_str() );
        }
    } while (FindNextFile(hFind, &win32fd));

    FindClose(hFind);

	return(image_files);
}

vector<string> readImageFiles(string path) {
	vector<string> image_files;

	image_files = readImageFiles(path, "bmp");
	if(image_files.size() > 0) { return(image_files); }

	image_files = readImageFiles(path, "BMP");
	if(image_files.size() > 0) { return(image_files); }

	image_files = readImageFiles(path, "jpg");
	if(image_files.size() > 0) { return(image_files); }

	image_files = readImageFiles(path, "JPG");
	if(image_files.size() > 0) { return(image_files); }

	image_files = readImageFiles(path, "png");
	if(image_files.size() > 0) { return(image_files); }

	image_files = readImageFiles(path, "PNG");
	if(image_files.size() > 0) { return(image_files); }

	if(image_files.size() == 0) {
		cerr << "There is no image in directory. " << path << endl;
		exit(0);
	}
	return(image_files);
}

////filenameの存在判定
//bool isExist(const string& filename) {
//    std::ifstream ifs(filename);
//    return ifs.is_open();
//}
////dirnameがディレクトリかどうかの判定
//bool existDirectory(char* dirname){
//	cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
//	return(false);
//}
//dirnameがない場合は作成する。
void prepareDirectory(string dirname) {
	cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
	return;
}

#else
//pathで指定するディレクトリから連番画像ファイル名のリストを読み込む。
vector<string> getImageFilesList(string path) {
	vector<string> image_files;
	DIR *dp;
	dirent* entry; // readdir() で返されるエントリーポイント

	dp = opendir(path.c_str());
	if (dp==NULL) {
		cerr << "Can't open directory " << path << endl;
		exit(1);
	}
	do {
		entry = readdir(dp);
		if (entry != NULL) {
			//			std::cout << path << entry->d_name << std::endl;
			if(strstr(entry->d_name, ".jpg") != NULL || strstr(entry->d_name, ".png") ||
					strstr(entry->d_name, ".NEF" ) != NULL || strstr(entry->d_name, ".nef" ) != NULL) {
//				image_files.push_back(cv::format("%s/%s", path.c_str(), entry->d_name));
				//cout << entry->d_name << endl;
				image_files.push_back(cv::format("%s%s", path.c_str(), entry->d_name));
			}
		}
	} while (entry != NULL);

	if(image_files.size() == 0) {
		cerr << "There is no image in directory. " << path << endl;
		exit(0);
	}
	vector<string> sorted_image_files = sortFileListByTime(image_files);
	return(sorted_image_files);
}

vector<string> getNEFFilesList(string path) {
	vector<string> image_files;
	DIR *dp;
	dirent* entry; // readdir() で返されるエントリーポイント

	dp = opendir(path.c_str());
	if (dp==NULL) {
		cerr << "Can't open directory " << path << endl;
		exit(1);
	}
	do {
		entry = readdir(dp);
		if (entry != NULL) {
			if(strstr(entry->d_name, ".NEF" ) != NULL || strstr(entry->d_name, ".nef" ) != NULL) {
				image_files.push_back(cv::format("%s%s", path.c_str(), entry->d_name));
			}
		}
	} while (entry != NULL);

	if(image_files.size() == 0) {
		cerr << "There is no image in directory. " << path << endl;
		exit(0);
	}
	vector<string> sorted_image_files = sortFileListByTime(image_files);
	return(sorted_image_files);
}

//
////filenameの存在判定
bool isExist(const string& filename) {
    std::ifstream ifs(filename);
    return ifs.is_open();
}
////dirnameがディレクトリかどうかの判定
//bool existDirectory(char* dirname){
//	struct stat st;
//	if(stat(dirname,&st) != 0){
//		return false;
//	}else{
//		mode_t m = st.st_mode;
//		if(S_ISDIR(m)){
//			return true;
//		}else{
//			return false;
//		}
//	}
//}
string replaceAll(string &str, string from, string to) {
    unsigned int pos = str.find(from);
    int toLen = to.length();

    if (from.empty()) { return(str); }

//    while ((pos = str.find(from, pos)) != string::npos) {
    while ((pos = str.find(from, pos)) <= str.length()) {
    	//cout << str << " " ;
    	str.replace(pos, from.length(), to);
    	//cout << str << endl;
        pos += toLen;
    }
    return str;
}
//dirnameを用意する
void prepareDirectory(string dirname) {
	//-pが効かないが\でパスが区切られていれば子ディレクトリまで作られる。
	string pathname = dirname;
	replaceAll(pathname, "/", "\\");

//	cout << dirname << " " << pathname << endl;
//	cout << isExist(dirname) << " " << isExist(pathname) << endl;
	if (opendir(pathname.c_str()) == NULL) {
		string cmdline = "mkdir " + pathname;
		cout << "#" << cmdline.c_str() << endl;
		system(cmdline.c_str());
	}
	else { cout << "#" << pathname << " already exists." << endl; }

	return;
}

#endif

//拡張子を含めたファイル名を返す
string getLeafname(const string fileName) {
	string basename;
	string separator = "/";
	string::size_type p1 = fileName.find_last_of(separator);
	string::size_type p2 = fileName.size();
	if(p1 > fileName.size()) { p1 = 0; }
	else { p1 = p1 + 1; }
	if(p1 < p2) {
		basename = fileName.substr(p1, p2-p1);
	}
	else {
		basename = "no_basename";
	}

	return(basename);

}

//fileNameの拡張子を返す。
string getExtension(const string fileName) {
	string extension;
	string::size_type p = fileName.find_last_of(".");
	//cout << p << " " << fileName.size() << endl;
	if(p < fileName.size()) {
		extension = fileName.substr(p);
	}
	else {
		extension = "no_extension";
	}

	return(extension);
}
//拡張子を除いたファイル名を返す。
string getBasename(const string fileName) {
	string basename;
	string separator = "/";
	string::size_type p1 = fileName.find_last_of(separator);
	string::size_type p2 = fileName.find_last_of(".");
	//cout << p1 << " " << p2 << endl;
	if(p1 > fileName.size()) { p1 = 0; }
	else { p1 = p1 + 1; }
	if(p2 > fileName.size()) { p2 = fileName.size(); }
	if(p1 < p2) {
		basename = fileName.substr(p1, p2-p1);
	}
	else {
		basename = "no_basename";
	}

	return(basename);
}
//TBD
//pathで指定するディレクトリから連番画像ファイル名のリストを読み込む。boost仕様版
//vector<boost::filesystem::path> getPathList(const boost::filesystem::path dir) {
//	//dirがディレクトリの場合、dir内のファイルパスをリストにして返す。
//	//dirがディレクトリではない場合は例外をスロー。
//	namespace bfs = boost::filesystem; //directory_iterator
//	vector<bfs::path> pathList;
//	bfs::directory_iterator end;
//
//	if(bfs::is_directory(dir) != true) {
//		throwUtilsException("Can't open directory. " + dir.string());
//	}
//
//	for (bfs::directory_iterator p(dir); p != end; ++p) {
//		if (bfs::is_directory(*p)) {  //ディレクトリの時
//			cerr << cv::format("#%s is directory.\n",p->path().string().c_str());
//		}
//		else {
//			if(p->path().extension() != ".db") {
//				pathList.push_back(p->path());
//				//			cout << boost::format("%s %s\n") %p->path().string() %p->path().leaf();
//			}
//		}
//	}
//
//	return(pathList);
//}

//-1,1,1,1
//のようにカンマで区切られた文字列coeffArgを解析してvectorに収めて返す。
template <class T>
vector<T> parseCoefficients(string coeffArg) {
	int countComma = 0;
	for(string::iterator it = coeffArg.begin(); it < coeffArg.end(); it++) { if(*it==',') countComma++; }
	vector<T> coeff(countComma+1);
	istringstream is(coeffArg);
	char c;

	for(typename vector<T>::iterator it = coeff.begin(); it < coeff.end(); it++) {
		is >> *it >> c;
	}
	return(coeff);
}

//TBD
//-1,1,1,1
//のようにカンマで区切られた文字列coeffArgを解析してvectorに収めて返す。boost仕様版
//template<> vector<string> parseCoefficients(string coeffArg) {
//	//-1,1,1,1
//	//のようにカンマで仕切られた係数の文字列を解析して返す。
//	typedef boost::char_separator<char> char_separator;
//	typedef boost::tokenizer<char_separator> tokenizer;
//
//	char_separator sep(",", "", boost::keep_empty_tokens);
//	tokenizer tokens(coeffArg, sep);
//	vector<string> coeff;
//	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
//		coeff.push_back(*tok_iter);
//	}
//
//	return(coeff);
//}

//-1,1,1,1
//のようにカンマで区切られた文字列coeffArgを解析してvectorに収めて返す。
//boost使用時にstringだけ特殊化した。統一可能か？
template<> vector<string> parseCoefficients(string coeffArg) {
	//-1,1,1,1
	//のようにカンマで仕切られた係数の文字列を解析して返す。
	vector<string> coeff;
	string::size_type pos = 0;
	string delim = ",";

	while(pos != string::npos ) {
		string::size_type p = coeffArg.find(delim, pos);

		if(p == string::npos) {
			coeff.push_back(coeffArg.substr(pos));
			break;
		}
		else {
			coeff.push_back(coeffArg.substr(pos, p - pos));
		}

		pos = p + delim.size();
	}

	return(coeff);
}

//表示用にvectorの要素を連結した文字列を返す
template <class T>
string vectorToString(vector<T> v) {
	ostringstream os;
	for(typename vector<T>::iterator it = v.begin(); it < v.end(); it++) {
//		os << cv::format("%s ",*it);
		os << *it << " " ;
	}
	return(os.str());
}

char **parseArgsFile(int* argc,char **argv) {
	//コマンドライン引数配列を解析して --args argsFile があった場合に、
	//argsFile内の#以下を除く文字列を並べてコマンドライン引数配列に追加する
	//DefocusContrastAFSimApp.cppでしか使ってないがもっと使っても良さそう

	static vector<string> args;
	for(int i = 0; i < *argc; i++) {
		if(string(argv[i]) == "--args") {
			i++;
			try {
				ifstream ifs(argv[i],ios::in);
				//if(ifs == NULL) {
				if(ifs.is_open() != true) {
					cerr << ERROR_LINE << "Can't open file. " << argv[i] << endl; exit(-1);
				}
				string line,arg1,arg2;
				while (getline(ifs, line) ) {
					if(line.at(0) == '#') { continue; }
					istringstream is(line);
					int s = 0;
					while(1) {
						string arg;
						is >> arg;
						if(arg != "" && arg.at(0) != '#') { args.push_back(arg); }
						else { break; }
						//						cout << arg << endl;
						s++;
						if(s >= 10) { cerr << ERROR_LINE << "Too many args? s >= 10" << endl; exit(-1);}
					}
				}
				ifs.close();
			} catch (std::exception &ex) {
				std::cerr << ex.what() << std::endl;
			}
			continue;
		}
		else {
			args.push_back(argv[i]);
			//			cout << i << " " << args.back() << endl;
		}
		//		cout << i << " " << args.back() << endl;
	}

	//	cout << "### " << args.size() << " " << endl;
	static char ** extendedArgv = (char **)calloc(args.size(),sizeof(char *));
	for(unsigned int i = 0; i < args.size(); i++) {
		extendedArgv[i] = const_cast<char *>(args[i].c_str());
		//		cout << i << " " << args[i] << " " << extendedArgv[i]<< endl;
	}
	*argc = (int)args.size();
	return(extendedArgv);
}

vector<string> sortFileListByTime(vector<string> files) {
	vector<string> sorted_files;
#ifdef USE_BOOST
		vector< pair<ptime, string> > time_files;
		for(unsigned int i = 0; i < files.size(); i++) {
			//cout << i << " " << filenames[i] << " ";
			try {
				const fs::path path(files[i]);
				const ptime time = boost::posix_time::from_time_t(fs::last_write_time(path));
				time_files.push_back(make_pair(time, files[i]));
			}
			catch (fs::filesystem_error& ex) {
				std::cout << "エラー発生！ : " << ex.what() << std::endl;
			}
		}

		std::sort(time_files.begin(), time_files.end());

		for(auto time_file:time_files) {
			sorted_files.push_back(time_file.second);
		}

#else
		vector< pair<ptime, string> > time_files;
		for(unsigned int i = 0; i < files.size(); i++) {
			const ptime time = fs::last_write_time(files[i]);
			time_files.push_back(make_pair(time, files[i]));
		}

		//		std::sort(time_filenames.begin(), time_filenames.end(), greater< pair< ptime, string > >());
		std::sort(time_files.begin(), time_files.end());

		for(auto time_file:time_files) {
			sorted_files.push_back(time_file.second);
		}

		//		cout << "#after sort" << std::endl;
//		for(auto time_filename:time_filenames) {
//			auto sec = chrono::duration_cast<chrono::seconds>(time_filename.first.time_since_epoch());
//			std::time_t t = sec.count();
//			const tm* lt = std::localtime(&t);
//			std::cout << time_filename.second << " : " << std::put_time(lt, "%c") << std::endl;
//		}
//		latest_filename = time_files[time_files.size() - 1 + last].second;
//		auto sec = chrono::duration_cast<chrono::seconds>(time_filenames[time_filenames.size() - 1 + last].first.time_since_epoch());
//		std::time_t t = sec.count();
//		const tm* lt = std::localtime(&t);
//		cout << cv::format("#Selected %dth latest captured file : %s : ",
//				last, latest_filename.c_str()) << std::put_time(lt, "%c") << endl;
#endif

	return sorted_files;
}

//dirでタイムスタンプが最後からlast番目のファイル名を返す
string getLatestNEFFile(const string dir, int last = 0) {
	string latest_filename;
	vector<string> filenames = getImageFilesList(dir);

#ifdef USE_BOOST
		vector< pair<ptime, string> > time_filenames;
		for(unsigned int i = 0; i < filenames.size(); i++) {
			//cout << i << " " << filenames[i] << " ";
			try {
				const fs::path path(filenames[i]);
				const ptime time = boost::posix_time::from_time_t(fs::last_write_time(path));
				time_filenames.push_back(make_pair(time, filenames[i]));
			}
			catch (fs::filesystem_error& ex) {
				std::cout << "エラー発生！ : " << ex.what() << std::endl;
			}
		}
		//		std::sort(time_filenames.begin(), time_filenames.end(), greater< pair< ptime, string > >());
		std::sort(time_filenames.begin(), time_filenames.end());
		cout << "#after sort" << std::endl;
//		for(auto time_filename:time_filenames) {
//			auto sec = chrono::duration_cast<chrono::seconds>(time_filename.first.time_since_epoch());
//			std::time_t t = sec.count();
//			const tm* lt = std::localtime(&t);
//			cout << time_filename.second << " : " << time_filename.first << endl;
//
//			//cout << time_filename.first << endl;
//		}
		latest_filename = time_filenames[time_filenames.size() - 1 + last].second;
		ptime time = time_filenames[time_filenames.size() - 1 + last].first;
		cout << cv::format("#Selected %dth latest captured file : %s ",
				last, latest_filename.c_str()) << time << endl;

#else
		vector< pair<ptime, string> > time_filenames;
		for(unsigned int i = 0; i < filenames.size(); i++) {
			const ptime time = fs::last_write_time(filenames[i]);
			time_filenames.push_back(make_pair(time, filenames[i]));
		}

		//		std::sort(time_filenames.begin(), time_filenames.end(), greater< pair< ptime, string > >());
		std::sort(time_filenames.begin(), time_filenames.end());
//		cout << "#after sort" << std::endl;
//		for(auto time_filename:time_filenames) {
//			auto sec = chrono::duration_cast<chrono::seconds>(time_filename.first.time_since_epoch());
//			std::time_t t = sec.count();
//			const tm* lt = std::localtime(&t);
//			std::cout << time_filename.second << " : " << std::put_time(lt, "%c") << std::endl;
//		}
		latest_filename = time_filenames[time_filenames.size() - 1 + last].second;
		auto sec = chrono::duration_cast<chrono::seconds>(time_filenames[time_filenames.size() - 1 + last].first.time_since_epoch());
		std::time_t t = sec.count();
		const tm* lt = std::localtime(&t);
		cout << cv::format("#Selected %dth latest captured file : %s : ",
				last, latest_filename.c_str()) << std::put_time(lt, "%c") << endl;
#endif

	return latest_filename;
}


//メモリ使用量とユーザ時間を表す文字列を返す。
string getTimeInfo() {
	//TODO VSでの実装確認/gccでの実装追加
#ifdef VS2012
	string usageInfo = "no usageInfo";
	clock_t endTime = clock();
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	DWORD dwProcessID = GetCurrentProcessId();
	HANDLE hProcess;
	if ( (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID)) != NULL ){
		if ( GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc)) ){
			usageInfo = cv::format("%8.3fs",(double)(endTime - startTime) / CLOCKS_PER_SEC);
		}
		else{
			cout << ERROR_LINE << " error " << endl; exit(0);
			//            printf( TEXT("%lu:GetProcessMemoryInfo()関数のエラーです。\n"), GetLastError() );
		}
		CloseHandle( hProcess );
	}
	else {
		cout << ERROR_LINE << " error " << endl; exit(0);
	}

	return(usageInfo);
#else
//	int who = RUSAGE_SELF;
//	struct rusage usage;
//	getrusage(who,&usage);
//	//	cout << usage.ru_maxrss << endl;
//	string usageInfo = cv::format("%8.3fs",usage.ru_utime.tv_sec+usage.ru_utime.tv_usec*1e-6);
	string usageInfo = "Unknown";
	return(usageInfo);
#endif
}

//メモリ使用量とユーザ時間を表す文字列を返す。
string getUsageInfo() {
//TODO VSでの実装確認/gccでの実装追加
#ifdef VS2012
	string usageInfo = "no usageInfo";
	clock_t endTime = clock();
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	DWORD dwProcessID = GetCurrentProcessId();
	HANDLE hProcess;
	if ( (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID)) != NULL ){
		if ( GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc)) ){
//			usageInfo = cv::format("WSS=%dMB %8.3fs",
//					pmc.WorkingSetSize/1024/1024,
//					(double)(endTime - startTime) / CLOCKS_PER_SEC);
			usageInfo = cv::format("WSS=%dMB",pmc.WorkingSetSize/1024/1024);
		}
		else{
			cout << ERROR_LINE << " error " << endl; exit(0);
			//            printf( TEXT("%lu:GetProcessMemoryInfo()関数のエラーです。\n"), GetLastError() );
		}
		CloseHandle( hProcess );
	}
	else {
		cout << ERROR_LINE << " error " << endl; exit(0);
	}

	return(usageInfo);
#else
//	int who = RUSAGE_SELF;
//	struct rusage usage;
//	getrusage(who,&usage);
//	//	cout << usage.ru_maxrss << endl;
//	//string usageInfo = cv::format("%8dMB %8.3fs",usage.ru_maxrss/1024,usage.ru_utime.tv_sec+usage.ru_utime.tv_usec*1e-6);
//	string usageInfo = cv::format("%8dMB",usage.ru_maxrss/1024);
	string usageInfo = "Unknown";
	return(usageInfo);
#endif
}
//メモリ使用量とユーザ時間を表す文字列を返す。
string getUsageInfoKB() {
	//TODO VSでの実装確認/gccでの実装追加
#ifdef VS2012
	string usageInfo = "no usageInfo";

	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	DWORD dwProcessID = GetCurrentProcessId();
	HANDLE hProcess;
	if ( (hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,dwProcessID)) != NULL ){
		if ( GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc)) ){
			usageInfo = cv::format("WSS=%dKB ",pmc.WorkingSetSize/1024);
		}
		else{
			cout << ERROR_LINE << " error " << endl; exit(0);
			//            printf( TEXT("%lu:GetProcessMemoryInfo()関数のエラーです。\n"), GetLastError() );
		}
		CloseHandle( hProcess );
	}
	else {
		cout << ERROR_LINE << " error " << endl; exit(0);
	}

	return(usageInfo);
#else
//	int who = RUSAGE_SELF;
//	struct rusage usage;
//	getrusage(who,&usage);
//	string usageInfo = cv::format("%12ldKB %8.3fs",usage.ru_maxrss,usage.ru_utime.tv_sec+usage.ru_utime.tv_usec*1e-6);
	string usageInfo = "Unknown";
	return(usageInfo);
#endif
}

template vector<short> parseCoefficients(string kernelArg);
template vector<unsigned int> parseCoefficients(string kernelArg);
template vector<int> parseCoefficients(string kernelArg);
template vector<float> parseCoefficients(string kernelArg);
template vector<double> parseCoefficients(string kernelArg);
template string vectorToString(vector<int> v);
template string vectorToString(vector<float> v);
template string vectorToString(vector<double> v);
template string vectorToString(vector<string> v);
}

