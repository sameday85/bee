#http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
OBJ = bee.o url.o 
CFLAGS=-lcurl
DEPS = common.h
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

bee: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
