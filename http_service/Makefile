MAIN_PATH := ../../../../
SRC_PATH := ../../../../src
COMMON_PATH := ../../../../src/common
PROJECT_PATH := ../../project
PROGRAM := ../../../../run/service/http_service/httpSrv

CHANNEL = channel/nduo/*.cpp
CHANNEL+= channel/uuc/*.cpp
CHANNEL+= channel/huawei/*.cpp
INC_PATH := -I/usr/local/include -I$(COMMON_PATH) -I$(PROJECT_PATH) -I$(SRC_PATH)/app/frame -I$(MAIN_PATH)/import/third_party/include/

LIB_PATH := -Wl,-Bstatic -L$(MAIN_PATH)/lib/ -L$(MAIN_PATH)/import/third_party/lib/
LIBS     := $(LIB_PATH) -ldb -lhiredis -lserviceFrame -lmessageComm -lprojcommon -lconnect -lbase -pthread -lprotobuf -lxerces-c -Wl,-Bdynamic -lssl -lcurl -liconv -lcrypto

CFLAGS = $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++
SRC=$(wildcard *.cc)
SRC+=$(wildcard *.cpp)
SRC+= $(wildcard $(CHANNEL))
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET= $(PROGRAM)

all: $(TARGET)
 
$(TARGET): $(OBJ)
	$(CC) -o2 -g -Wall -o $(TARGET) $(OBJ) $(LIBS)
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
  
%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJ)
	



