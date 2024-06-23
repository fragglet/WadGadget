
LIBSIXEL_CFLAGS = \
    $(shell pkg-config --silence-errors --cflags libsixel \
   && echo -DHAVE_LIBSIXEL)
LIBSIXEL_LDFLAGS = \
    $(shell pkg-config --silence-errors --libs libsixel)

# Ideally we use the version of ncurses pkg-config tells us about, but
# some systems (eg. BSDs) ship ncurses as part of the base OS without the
# accompanying .pc file. So fall back to just providing -lncurses.
NCURSES_CFLAGS = \
    $(shell pkg-config --silence-errors --cflags ncurses)
NCURSES_LDFLAGS = \
    $(shell pkg-config --silence-errors --libs ncurses || echo -lncurses)

REQUIRED_PKGS = sndfile ncurses libpng
CFLAGS = -g -MMD -Wall -I. -O0 $(shell pkg-config --cflags $(REQUIRED_PKGS)) \
         $(LIBSIXEL_CFLAGS) $(NCURSES_CFLAGS)
LDFLAGS = $(shell pkg-config --libs $(REQUIRED_PKGS)) \
          $(LIBSIXEL_LDFLAGS) $(NCURSES_LDFLAGS)

OBJS =                      \
    conv/audio.o            \
    conv/endoom.o           \
    conv/error.o            \
    conv/export.o           \
    conv/graphic.o          \
    conv/import.o           \
    conv/mus2mid.o          \
    fs/file_set.o           \
    fs/real_dir.o           \
    fs/vfile.o              \
    fs/vfs.o                \
    fs/wad_dir.o            \
    fs/wad_file.o           \
    textures/actions.o      \
    textures/bundle.o       \
    textures/config.o       \
    textures/lump_dir.o     \
    textures/pnames.o       \
    textures/pnames_dir.o   \
    textures/texture_dir.o  \
    textures/textures.o     \
    ui/actions_pane.o       \
    ui/dialog.o             \
    ui/directory_pane.o     \
    ui/list_pane.o          \
    ui/pane.o               \
    ui/text_input.o         \
    ui/ui.o                 \
    actions.o               \
    lump_info.o             \
    sixel_display.o         \
    stringlib.o             \
    struct.o                \
    termfuncs.o             \
    view.o                  \
    wadgadget.o

DEPS = $(patsubst %.o,%.d,$(OBJS))

wadgadget : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f wadgadget $(OBJS) $(DEPS)

-include $(DEPS)
