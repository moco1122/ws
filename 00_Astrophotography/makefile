
export PATH := /usr/local/bin/:$(PATH)
#:で区切る
export PKG_CONFIG_PATH := /usr/local/opt/opencv@2/lib/pkgconfig:/usr/local/opt/gflags/lib/pkgconfig

#INCL=-I/opt/local/include
#LIBS=-L/opt/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_program_options

CXX=g++
CXX_FLAG=-Wall -O3

#INCL_CV=`pkg-config --cflags opencv`
#LIBS_CV=`pkg-config --libs opencv` -ljpeg -ljasper -llcms2 
INCL_CV=/usr/local/Cellar/opencv@2/2.4.13.6_2/include/opencv:/usr/local/Cellar/opencv@2/2.4.13.6_2/include
CV_LIB=/usr/local/Cellar/opencv@2/2.4.13.6_2/lib
CV_LD_FLAG=-lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_nonfree -lopencv_objdetect -lopencv_ocl -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab -ljpeg -ljasper -llcms2

#INCL_GF=`pkg-config --cflags gflags`
#LIBS_GF=`pkg-config --libs gflags`
INCL_GF=/usr/local/Cellar/gflags/2.2.1/include
GF_LIB=/usr/local/Cellar/gflags/2.2.1/lib
GF_LD_FLAG=-lgflags

R_DIR=/Library/Frameworks/R.framework
INCL_R=${R_LIB}/Resources/include:${R_LIB}/Rcpp/include:${R_LIB}/RInside/include
R_LIB=${R_DIR}/Versions/3.5/Resources/library

export C_INCLUDE_PATH=${INCL_CV}:${INCL_GF}:${INCL_R}
export CPLUS_INCLUDE_PATH=${INCL_CV}:${INCL_GF}:${INCL_R}
export LIBRARY_PATH=/usr/local/lib:${CV_LIB}:${GF_LIB}:${R_LIB}/RInside/lib

LD_FLAG=${CV_LD_FLAG} ${GF_LD_FLAG} ${R_LD_FLAG}

test : Test.cpp makefile
	echo ${LIBRARY_PATH}
	g++ -o $@ Test.cpp ${LD_FLAG}
	cp $@ ../bin/
		
#nefreadertest : NEFReader_test.cpp NEFReader.cpp NEFReader.hpp dcraw_functions.h dcraw_functions.o
#	${CXX} ${CXX_FLAG} -c -o NEFReader.o NEFReader.cpp
#	${CXX} ${CXX_FLAG} -o $@ NEFReader_test.cpp NEFReader.o dcraw_functions.o
#	# ${LIBS} -ljasper -ljpeg -llcms2
#	cp $@ /ws/bin/
#dcraw_functions.o : dcraw_functions.c dcraw_functions.h
#	${CXX} ${CXX_FLAG} -c -o $@ dcraw_functions.c
#FLAGS=-O3 -Wall -Wno-sign-compare -Wno-dangling-else -Wno-constant-conversion -Wno-macro-redefined
#FLAGS+=-Wno-c++11-compat-deprecated-writable-strings -Wno-constant-logical-operand 

Wno_FLAG+=-Wno-dangling-else -Wno-constant-conversion -Wno-macro-redefined
Wno_FLAG+=-Wno-c++11-compat-deprecated-writable-strings -Wno-constant-logical-operand 
Wno_FLAG+=-Wno-string-plus-int -Wno-unused-variable

dcrawcv : dcrawcvApp.cpp makefile
	${CXX} ${CXX_FLAG} -o $@ dcrawcvApp.cpp ${LD_FLAG} ${Wno_FLAG}
	cp $@ ../bin/

gflagtest : gflag.cpp makefile
	${CXX} ${CXX_FLAG} -o $@ gflag.cpp ${GF_LD_FLAG}
	cp $@ ../bin/
	
#サンプルのコンパイルオプション 残しとく
#R_LD_FLAG=-F/Library/Frameworks/R.framework/.. -framework R -lpcre -llzma -lbz2 -lz -licucore -lm -liconv  -L/Library/Frameworks/R.framework/Resources/lib -lRblas -L/Library/Frameworks/R.framework/Resources/lib -lRlapack  /Library/Frameworks/R.framework/Versions/3.5/Resources/library/RInside/lib/libRInside.a

#R_LD_FLAG=-lR #これだとSegmentation Fault
R_LD_FLAG=-F/Library/Frameworks/R.framework/.. -framework R
#下は外してもOKだった->　使う関数によっては必要かもなので残しとく
# -lpcre -llzma -lbz2 -lz -licucore -lm -liconv  -L/Library/Frameworks/R.framework/Resources/lib -lRblas　-L/Library/Frameworks/R.framework/Resources/lib -lRlapack  
R_LD_FLAG+=/Library/Frameworks/R.framework/Versions/3.5/Resources/library/RInside/lib/libRInside.a

rtest : makefile ../RInside/standard/rinside_sample0.cpp 
	${CXX} ${CXX_FLAG} -o $@ ../RInside/standard/rinside_sample0.cpp ${R_LD_FLAG}	
	cp $@ ../bin/
	
check_path :
	echo ${INCL_CV}
	echo ${INCL_GF}
	echo ${LIBS_CV}
	echo ${LIBS_GF}
	echo ${LD_FLAG}
	@env | grep ^PATH
	@env | grep INCLUDE
	@env | grep LIBRARY
	@which gcc
	@which g++
	cp $@ ../bin/
	
clean : 
	rm -f *.o	
