# task
# cloc - count line of source code.

# dependencies
#
# on develop
#   $ apt install cloc doxygen dot2tex
#   gcc or clang
#   make, efence, etags, clang-format
#   gdb

CC = clang-12
#CC = clang

# _POSIX_C_SOURCE: fdopen(3)
# _DEFAULT_SOURCE: timezone
# refer to feature_test_macros(7)
CFLAGS = -g -Wall -std=c17 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

# efence: electric fence
# libc: fdopen(3)
#LIBS = -lefence -lc
LIBS = -lc 

LDFLAGS = -fuse-ld=mold

TARGET = httpd
TEST   = test
SRCS = main.c server.c net.c file.c util.c util_test.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format docs tags cloc check

all: $(TARGET)

clean:
	- rm -f *~ a.out $(TARGET) $(TEST) $(OBJS) 

format:
	clang-format -i *.[ch] 

docs:
	doxygen

tags:
	etags $(SRCS) *.h

cloc:
	cloc $(SRCS) *.h

check: $(TARGET) $(TEST)
	./$(TARGET) -test
	./$(TEST)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

main.o:      util.h file.h net.h main.h       
server.o:    util.h file.h net.h main.h  
file.o:      util.h file.h
net.o:       util.h        net.h 
util.o:      util.h
util_test.o: util.h
