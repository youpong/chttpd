#CC = clang

# _POSIX_C_SOURCE: fdopen(3)
# _DEFAULT_SOURCE: timezone
CFLAGS = -g -Wall -std=c18 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
# refer to feature_test_macros(7)

# libc: fdopen(3)
LIBS = -lc

TARGET = httpd
SRCS = main.c worker.c net.c util.c util_test.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format check tags

all: $(TARGET)

clean:
	- rm -f a.out $(TARGET) $(OBJS) *~ test

format:
	clang-format -i *.c

tags:
	etags $(SRCS) *.h

check: $(TARGET)
	./$(TARGET) -test

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

# following doesn't work... why?
# $(OJBS): httpd.h
main.o: main.h
worker.o: main.h net.h
net.o: net.h
