CFLAGS = -Wall -Itmpincl

all: wadview.o

%.o : %.c
	$(CC) $(CFLAGS) $^ -o $@

