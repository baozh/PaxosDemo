LIB_MUDUO_COMM_PATH = ../lib
LIB_MUDUO_COMM = -L$(LIB_MUDUO_COMM_PATH) -lmuduo -lpthread -lrt

LIB_BOOST_COMM := -L/usr/lib/x86_64-linux-gnu/ -lboost_thread -lboost_system

LIB_COMMON_SERVER_COMM_PATH = ../lib
LIB_COMMON_SERVER_COMM = -L$(LIB_MUDUO_COMM_PATH) -lcomm_serv

INC_COMM_DIR =  -I. -I../include/ -I../

LIB_COMM = $(LIB_COMMON_SERVER_COMM) $(LIB_MUDUO_COMM) -isystem $(LIB_BOOST_COMM)  -L../lib -ljsoncpp

C_ARGS = -g -Wall $(INC_COMM_DIR)

BINARY := paxosdemo

CXX = g++
OI_OBJS := main.o PaxosCommProto.o  ProcessMsgHandler.o  ./ini/stringutil.o ./ini/inifile.o 

all:$(BINARY)

.cc.o:
	$(CXX) $(C_ARGS)  -c $^ -o $@  -DDEAMON -DMUDUO_STD_STRING

.cpp.o:
	$(CXX) $(C_ARGS)  -c $^ -o $@  -DDEAMON -DMUDUO_STD_STRING

$(BINARY):$(OI_OBJS)
	$(CXX) $(C_ARGS) -o $@ $^ $(LIB_COMM)


clean:
	rm -f *.o $(BINARY) *.log ../ini/*.o

