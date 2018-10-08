/*
 * NEFReader_test.cpp
 *
 *  Created on: 2015/10/09
 *      Author: kawai
 */

#include <iostream>
#include "NEFReader.hpp"
using namespace std;

int main(int argc, char **argv) {
	if(argc < 2) {
		cerr << "Usage : * file.NEF" << endl;
		exit(0);
	}
	NEFReader nr(argv[1]);
//あああ
	cout << nr.nef.aaa << endl;
	return(0);
}
