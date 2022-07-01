CFLAGS = -Wall -Itmpincl

all: wadview.o

%.o : %.c
	$(CC) -c $(CFLAGS) $^ -o $@

