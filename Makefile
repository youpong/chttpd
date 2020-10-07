CC = clang
CFLAGS = -g -Wall -std=c18

TARGET = httpd
SRCS = main.c worker.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format check 

all: $(TARGET)

clean:
	- rm -f a.out *.o $(TARGET) test

format:
	clang-format -i *.c

check: all test client
	./test

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

# following doesn't work... why?
# $(OJBS): httpd.h
main.o: httpd.h
worker.o: httpd.h
