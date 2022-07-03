LDFLAGS = -lncurses
OBJS = wadgadget.o wad_file.o wad_pane.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

