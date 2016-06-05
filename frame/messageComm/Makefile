INC_PATH := -I/usr/local/include -I../../../common
#LIB_PATH := -L../../../../lib/
#LIBS     := $(LIB_PATH) -lbase -lconnect

CFLAGS = -static $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET=../../../../lib/libmessageComm.a

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

