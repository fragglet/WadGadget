CFLAGS = -Wall -Wno-unused-function -Itmpincl

OBJS = wadview.o prints.o impexp.o setpal.o gifpcx.o pnames.o \
       addclean.o filepart.o texture.o sg.o

all: rwt

rwt : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) rwt

