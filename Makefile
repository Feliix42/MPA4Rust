CXX=clang++
LD=clang++
CXXFLAGS=`llvm-config --cxxflags` -g
LDFLAGS=`llvm-config --ldflags` -lboost_filesystem -lboost_system
LLVMLIBS=`llvm-config --system-libs --libs`

MAIN = rmpa
SRCS = main.cpp scanner.cpp matching.cpp visualizer.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean debug

all: ${OBJS}
	${CXX} ${CXXFLAGS} -o ${MAIN} ${OBJS} ${LDFLAGS} ${LLVMLIBS}


.cpp.o: 
	${CXX} ${CXXFLAGS} -c $< -o $@

# myprog: ${OBJECTS}
#     ${LD} ${LDFLAGS} -o $@ ${OBJECTS} ${LIBS}

clean:
	$(RM) *.o *~ $(MAIN)


