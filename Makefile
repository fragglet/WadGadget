CFLAGS = -g -Wall -O0

LDFLAGS = -lncurses
OBJS = wadgadget.o wad_file.o vfile.o pane.o ui.o dialog.o text_input.o \
       lump_info.o strings.o import.o struct.o list_pane.o vfs.o \
       directory_pane.o actions_pane.o export.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f wadgadget $(OBJS)

