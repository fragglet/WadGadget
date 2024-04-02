CFLAGS = -g -Wall

LDFLAGS = -lncurses
OBJS = wadgadget.o wad_file.o blob_list_pane.o wad_pane.o dir_pane.o \
	vfile.o dir_list.o blob_list.o pane.o ui.o dialog.o \
	text_input.o lump_info.o strings.o import.o export.o \
	struct.o list_pane.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f wadgadget $(OBJS)

