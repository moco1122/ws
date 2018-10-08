
include Makefile_osx.in
include Makefile_win10.in

export C_INCLUDE_PATH=${INCL_CV}:${INCL_GF}:${INCL_R}
export CPLUS_INCLUDE_PATH=${INCL_CV}:${INCL_GF}:${INCL_R}
export LIBRARY_PATH=/usr/local/lib:${CV_LIB}:${GF_LIB}:${R_LIB}/RInside/lib

LD_FLAG=${CV_LD_FLAG} ${GF_LD_FLAG} ${R_LD_FLAG} -ltiff

LIB_MYCV=libmycv.a
OBJ_MYCV=obj/Utils++.o obj/MatUtils.o obj/QuickLook.o
# obj/Astro.o templateがあるのでだめ
${LIB_MYCV} : ${OBJ_MYCV} obj/dcrawFunc.o 
	ar r $@ ${OBJ_MYCV} obj/dcrawFunc.o 
	ranlib $@
${OBJ_MYCV} : 
	g++ -O3 -o $@ -c ${@:obj/%.o=%.cpp} -Wno-format
obj/Utils++.o : Utils++.cpp Utils++.hpp 
obj/MatUtils.o : MatUtils.cpp MatUtils.hpp 
obj/QuickLook.o : QuickLook.cpp QuickLook.hpp
#obj/Astro.o : Astro.cpp Astro.hpp

obj/dcrawFunc.o : dcrawFunc.cpp dcrawFunc.hpp
	g++ -O3 -o $@ -c ${@:obj/%.o=%.cpp} ${Wno_FLAG}
	
test : test.cpp makefile ${LIB_MYCV}
	g++ -O3 -o $@ test.cpp ${LIB_MYCV} ${LD_FLAG}
	cp $@ ../bin/

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
	
listnef : listnef.cpp makefile ${LIB_MYCV} 
	${CXX} ${CXX_FLAG} -o $@ listnef.cpp ${LIB_MYCV} ${LD_FLAG} ${Wno_FLAG}
	cp $@ ../bin/	
mkflat : mkflat.cpp makefile ${LIB_MYCV} 
	${CXX} ${CXX_FLAG} -o $@ mkflat.cpp ${LIB_MYCV} ${LD_FLAG} ${Wno_FLAG}
	cp $@ ../bin/
qlnef : qlnef.cpp makefile ${LIB_MYCV} Astro.hpp
	${CXX} ${CXX_FLAG} -o $@ qlnef.cpp ${LIB_MYCV} ${LD_FLAG} ${Wno_FLAG}
	cp $@ ../bin/

drawbox : drawbox.cpp makefile ${LIB_MYCV} 
	${CXX} ${CXX_FLAG} -o $@ drawbox.cpp ${LIB_MYCV} ${LD_FLAG} ${Wno_FLAG}
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
	
clean : 
	rm -f *.o obj/*.o *.a
