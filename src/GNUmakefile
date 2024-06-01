
LIBSIXEL_CFLAGS = \
    $(shell pkg-config --silence-errors --cflags libsixel \
   && echo -DHAVE_LIBSIXEL)
LIBSIXEL_LDFLAGS = \
    $(shell pkg-config --silence-errors --libs libsixel)

REQUIRED_PKGS = audiofile ncurses libpng
CFLAGS = -g -Wall -O0 $(shell pkg-config --cflags $(REQUIRED_PKGS)) \
         $(LIBSIXEL_CFLAGS)
LDFLAGS = $(shell pkg-config --libs $(REQUIRED_PKGS)) $(LIBSIXEL_LDFLAGS)

OBJS = wadgadget.o wad_file.o vfile.o pane.o ui.o dialog.o text_input.o \
       lump_info.o stringlib.o import.o struct.o list_pane.o vfs.o \
       directory_pane.o actions_pane.o export.o audio.o mus2mid.o \
       sixel_display.o graphic.o view.o endoom.o termfuncs.o actions.o \
       textures.o

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f wadgadget $(OBJS)

