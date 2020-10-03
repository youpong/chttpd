CC = clang
CFLAGS = -g -Wall -std=c18

TARGET = httpd
OBJS = main.o worker.o

.PHONY: all clean format check 

all: $(TARGET)

clean:
	- rm -f a.out *.o $(TARGET) test

format:
	clang-format -i *.c

check: all test client
	./test

$(OJBS): httpd.h
$(TARGET): $(OBJS)
	$(CC) -o $@ $^
