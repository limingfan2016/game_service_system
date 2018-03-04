MAIN_PATH := ../../../../
SRC_PATH := ../../../../src
HTTP_BASE_PATH := ../../../../src/http_base
COMMON_PATH := ../../../../src/common
PROJECT_PATH := ../../project
PROGRAM := ../../../../run/service/http_operation/httpOperation

INC_PATH := -I/usr/local/include -I$(HTTP_BASE_PATH) -I$(COMMON_PATH) -I$(PROJECT_PATH) -I$(SRC_PATH)/app/frame -I$(MAIN_PATH)/import/third_party/include/

LIB_PATH := -Wl,-Bstatic -L$(MAIN_PATH)/lib/ -L$(MAIN_PATH)/import/third_party/lib/
LIBS   := $(LIB_PATH) -lprojcommon -lserviceFrame -lhttpbase -lmessageComm -ldb -lconnect -lbase -pthread -lhiredis -lprotobuf -lxerces-c -Wl,-Bdynamic -lssl -lcrypto

CFLAGS = $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++

PROTOCOL = protocol/*.cc
SRC=$(wildcard *.cpp)
SRC+= $(wildcard $(PROTOCOL))

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
	



