CFLAGS = -static -I/usr/local/include -I../../../import/third_party/include  -I../../../import/third_party/include/mysql -I../../common -g -Wall -o2 -std=c++0x
CFLAGS +=   #调试日志开关，编译运行版本的时候注释掉该选项
CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(addprefix ./, $(addsuffix .o, $(basename $(SRC))))
TARGET=../../../lib/libdb.a

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
