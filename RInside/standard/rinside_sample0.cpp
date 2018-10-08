// -*- mode: C++; c-indent-level: 4; c-basic-offset: 4;  tab-width: 8; -*-
//
// Simple example showing how to do the standard 'hello, world' using embedded R
//
// Copyright (C) 2009 Dirk Eddelbuettel
// Copyright (C) 2010 Dirk Eddelbuettel and Romain Francois
//
// GPL'ed
#include <iostream>

#include <RInside.h>                    // for the embedded R via RInside

using namespace std;


int main(int argc, char *argv[]) {
	cout << "hello" << std::endl;
	for(int i = 0; i < argc; i++) {
		cout << i << " " << argv[i] << endl;
	}
    cout << __LINE__ << endl;
    RInside R(argc, argv);              // create an embedded R instance
    cout << __LINE__ << endl;
    R["txt"] = "Hello, world!";	// assign a char* (string) to 'txt'
    cout << __LINE__ << endl;

    R.parseEvalQ("cat(txt)");           // eval the init string, ignoring any returns
    cout << __LINE__ << endl;

    exit(0);
}

