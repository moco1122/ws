#include <gflags/gflags.h>
#include <iostream>
#include <fstream>
#include <string>

//フラグの定義
DEFINE_string(filename, "", "file name to write or read");
DEFINE_bool(read, false, "read file and show text");
DEFINE_string(append_text, "", "append text to the file");

/**
 * ファイルの内容を標準出力する
 */
int read_text(const std::string& filename) {
  std::ifstream ifs(filename.c_str());
  if (!ifs) {
    std::cerr << "Failed to open " << filename << std::endl;
    return 1;
  }
  std::string buf;
  while (std::getline(ifs, buf)) {
    std::cout << buf << std::endl;
  }
  return 0;
}

/**
 * ファイルに文字列を追加する
 */
int append_text(const std::string& filename, const std::string& append_text) {
  std::ofstream ofs(filename.c_str(), std::ios::app);
  if (!ofs) {
    std::cerr << "Failed to open " << filename << std::endl;
    return 1;
  }
  ofs << append_text << std::endl;
  return 0;
}

/** メイン関数 */
int main(int argc, char* argv[]) {
  // gflagsで定義したフラグを取り除く
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  int return_value = 0;

  //フラグごとの処理
  if (!FLAGS_filename.empty()) {
    if (FLAGS_read) {
      return_value = read_text(FLAGS_filename);
    } else {
      return_value = append_text(FLAGS_filename, FLAGS_append_text);
    }
  } else {
    std::cerr << "Error: Use --filename flag to give a file name" << std::endl;
    return_value = 2;
  }

  return return_value;
}
