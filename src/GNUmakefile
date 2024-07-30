
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

REQUIRED_PKGS = sndfile libpng
CFLAGS := -g -MMD -Wall -I. -O2 $(shell pkg-config --cflags $(REQUIRED_PKGS)) \
          $(LIBSIXEL_CFLAGS) $(NCURSES_CFLAGS)
LDFLAGS := $(shell pkg-config --libs $(REQUIRED_PKGS)) \
           $(LIBSIXEL_LDFLAGS) $(NCURSES_LDFLAGS)

HELP_FILES =                \
    ../COPYING.md           \
    help/common.md          \
    help/contents.md        \
    help/dir_view.md        \
    help/pnames_editor.md   \
    help/texture_editor.md  \
    help/wad_view.md        \
    help/hexen_specs.md     \
    help/help.md            \
    help/uds.md             \
    help/boomref.md         \
    help/mbfedit.md         \
    help/hexdump.md

OBJS =                      \
    browser/actions.o       \
    browser/actions_pane.o  \
    browser/browser.o       \
    browser/directory_pane.o\
    conv/audio.o            \
    conv/endoom.o           \
    conv/error.o            \
    conv/export.o           \
    conv/graphic.o          \
    conv/import.o           \
    conv/mus2mid.o          \
    conv/palette.o          \
    conv/vpng.o             \
    fs/file_set.o           \
    fs/real_dir.o           \
    fs/vfile.o              \
    fs/vfs.o                \
    fs/wad_dir.o            \
    fs/wad_file.o           \
    pager/help.o            \
    pager/hexdump.o         \
    pager/pager.o           \
    pager/plaintext.o       \
    palette/actions.o       \
    palette/doom.o          \
    palette/palette.o       \
    palette/palfs.o         \
    textures/actions.o      \
    textures/bundle.o       \
    textures/config.o       \
    textures/lump_dir.o     \
    textures/pnames.o       \
    textures/pnames_dir.o   \
    textures/texture_dir.o  \
    textures/textures.o     \
    ui/actions_bar.o        \
    ui/dialog.o             \
    ui/list_pane.o          \
    ui/pane.o               \
    ui/stack.o              \
    ui/text_input.o         \
    ui/title_bar.o          \
    ui/ui.o                 \
    help_text.o             \
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

help_text.c: $(HELP_FILES) help/make_help.py
	help/make_help.py $(HELP_FILES) > $@

clean :
	rm -f wadgadget $(OBJS) $(DEPS) help_text.c

-include $(DEPS)
