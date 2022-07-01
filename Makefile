CFLAGS = -Wall -Wno-unused-function -Itmpincl

OBJS = wadview.o prints.o impexp.o setpal.o gifpcx.o

all: rwt

rwt : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) rwt

