INSTALL_LIB_PATH = ../lib/

LIB_MUDUO_COMM_PATH =../lib
LIB_MUDUO_COMM = -L$(LIB_MUDUO_COMM_PATH) -lmuduo -lpthread -lrt

LIB_BOOST_COMM := -lboost_thread -lboost_system

INC_COMM_DIR = -I. -I../include/ -I../

LIB_COMM = $(LIB_MUDUO_COMM) -isystem $(LIB_BOOST_COMM) -L../lib -ljsoncpp

CC = g++
AR = ar

CFLAGS = -w -g -Wall -fno-common $(INC_COMM_DIR) 

HEADERS = $(wildcard *.h)

OBJS =  CommHeartbeat.o CommProto.o CommUtil.o JsonValue.o CommonServer.o MsgHandler.o PacketBuffer.o

LIB = libcomm_serv
OUTPUT_STATIC = $(LIB).a

.cc.o: $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $^  $(LIB_COMM) -DMUDUO_STD_STRING

all: $(OUTPUT_STATIC)

$(OUTPUT_STATIC):$(OBJS)
	$(AR) rv $@ $?
	
install:
	-mkdir -p $(INSTALL_LIB_PATH)
	-cp -f $(LIB)* $(INSTALL_LIB_PATH)

uninstall:
	-rm -f $(INSTALL_LIB_PATH)$(OUTPUT_STATIC)

clean:
	rm -f $(LIB)* *.o

