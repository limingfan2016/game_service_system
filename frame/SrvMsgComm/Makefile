INC_PATH := -I/usr/local/include -I../../../common -I../../frame
LIB_PATH := -L../../../../lib/
LIBS     := $(LIB_PATH) -lmessageComm -lconnect -lbase -pthread

CFLAGS = $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET= ../../../../run/platform/serviceMessageComm/SrvMsgComm

all: $(TARGET)
 
$(TARGET): $(OBJ)
	$(CC) -o2 -g -Wall -o $(TARGET) $(OBJ) $(LIBS)
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
 
%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJ)


