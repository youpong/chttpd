CC = clang
CFLAGS = -g -Wall -std=c18

TARGET = httpd

.PHONY: all clean format check 

all: $(TARGET)

clean:
	- rm -f a.out *.o $(TARGET) test

format:
	clang-format -i *.c

check: all test client
	./test

$(TARGET): main.o
	$(CC) -o $@ $^
