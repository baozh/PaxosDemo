INC_COMM_DIR = -I../include/
INSTALL_LIB_PATH = ../lib/

INC_COMM_DIR = -I../ -I../include

CC = g++
AR = ar

CFLAGS = -g $(INC_COMM_DIR) 

BASE_HEADERS = $(wildcard base/*.h)
NET_HEADERS = $(wildcard net/*.h)

#
MUDUO_DIRS = $(shell find . -maxdepth 5 -type d)
MUDUO_OBJS = $(patsubst %.cc,%.o,$(foreach dir,$(MUDUO_DIRS),$(wildcard $(dir)/*.cc)))

LIB = libmuduo
OUTPUT_STATIC = $(LIB).a

.cc.o:
	$(CC) $(CFLAGS) -o $@ -c $^  -DMUDUO_STD_STRING

all: $(OUTPUT_STATIC) 

$(OUTPUT_STATIC):$(MUDUO_OBJS) $(HEADERS)
	$(AR) rv $@ $?
	
install:
	-mkdir -p $(INSTALL_LIB_PATH)
	-cp -f $(LIB)* $(INSTALL_LIB_PATH)

uninstall:
	-rm -f $(INSTALL_LIB_PATH)$(OUTPUT_STATIC)

clean:
	rm -f $(LIB)* */*.o
		
	

