CFLAGS = -g -Wall -O0 $(shell pkg-config --cflags audiofile)
LDFLAGS = -lncurses $(shell pkg-config --libs audiofile)

OBJS = wadgadget.o wad_file.o vfile.o pane.o ui.o dialog.o text_input.o \
       lump_info.o strings.o import.o struct.o list_pane.o vfs.o \
       directory_pane.o actions_pane.o export.o audio.o mus2mid.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f wadgadget $(OBJS)

