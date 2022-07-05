CFLAGS = -g

LDFLAGS = -lncurses
OBJS = wadgadget.o wad_file.o list_pane.o wad_pane.o dir_pane.o \
       vfile.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

