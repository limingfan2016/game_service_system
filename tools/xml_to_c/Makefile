MAIN_PATH := ../../import/third_party/
INC_PATH := -I/usr/local/include -I$(MAIN_PATH)/include
LIB_PATH := -L$(MAIN_PATH)/lib/
LIBS     := -Wl,-Bstatic $(LIB_PATH) -lxerces-c -Wl,-Bdynamic -lcurl -liconv -pthread

CFLAGS = $(INC_PATH) -g -Wall -o2 -std=c++0x
CC=g++
SRC=$(wildcard *.cc)
SRC+=$(wildcard *.cpp)
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET= ./xmlToC++

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


