INC_PATH := -I/usr/local/include -I../../../common -I../../frame -I../../project
#LIB_PATH := -L../../../../lib/
#LIBS     := $(LIB_PATH) -lbase -lmessageComm

CFLAGS = -static $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET=../../../../lib/libserviceFrame.a

all: $(TARGET)
 
$(TARGET): $(OBJ)
	$(AR) -rcs $@ $^
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
 
%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJ)
	
