#CC = clang

# _POSIX_C_SOURCE: fdopen(3)
# _DEFAULT_SOURCE: timezone
CFLAGS = -g -Wall -std=c18 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
# refer to feature_test_macros(7)

# libc: fdopen(3)
LIBS = -lc

TARGET = httpd
SRCS = main.c server.c net.c util.c util_test.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format check tags

all: $(TARGET)

clean:
	- rm -f *~ a.out $(TARGET) $(OBJS) 

format:
	clang-format -i *.c

tags:
	etags $(SRCS) *.h

check: $(TARGET)
	./$(TARGET) -test

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

main.o:      main.h       util.h
server.o:    main.h net.h util.h
net.o:              net.h util.h
util.o:                   util.h
util_test.o:              util.h
