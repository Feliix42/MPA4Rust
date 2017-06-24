CXX=clang++
CXXFLAGS=`llvm-config --cxxflags`
LDFLAGS=`llvm-config --ldflags`
LLVMLIBS=`llvm-config --system-libs --libs`

.PHONY: all clean

# .cpp.o: 
# 	${CXX} ${CXXFLAGS} ${CXX_EXTRA} -c -o $@ $<

# myprog: ${OBJECTS}
#     ${LD} ${LDFLAGS} -o $@ ${OBJECTS} ${LIBS}

all:
	${CXX} ${CXXFLAGS} ${LDFLAGS} ${LLVMLIBS} -g -o main main.cpp

clean:
	rm -rf ./main


