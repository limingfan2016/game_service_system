CFLAGS = -static -I/usr/local/include -I../../../../import/third_party/include -I../../../common -I../../project -I../../../app/frame -g -Wall -o2 -std=c++0x
CC=g++

PROTOCOL = protocol/*.cc
SRC=$(wildcard *.cpp)
SRC+= $(wildcard $(PROTOCOL))

OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET=../../../../lib/libgameobject.a

all: $(TARGET)
 
$(TARGET): $(OBJ)
	$(AR) -rcs $@ $^
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	
%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<
 
%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJ)
