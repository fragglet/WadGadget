# WAD file editor

WAD files ("Where's All the Data?") contain all of the artwork, levels, sound
effects and music used by Doom. Doom mods are usually constructed as "Patch WAD"
(PWAD) files that replace some of the lumps within the main "IWAD" file (which
is usually named doom.wad, doom2.wad or similar depending on the game). You
should usually not be modifying the IWAD file.

The WAD editor allows WAD files to be created and edited. Most importantly it
allows new resources to be [imported](#copying) into the WAD from files, or to be copied
from other WAD files. Each WAD file consists of a number of named "lumps"; they
contain different kinds of data depending on their purpose.

It is possible to navigate inside of certain lump types (the [texture editor](texture_editor.md)
and [PNAMES editor](pnames_editor.md)). Selecting the first entry in the WAD
list returns to the directory containing the WAD.

## Keys

    **        Enter   **  View/edit lump
    **Ctrl-D          **  View hexdump of selected lump
    **Ctrl-V  F2      **  Move (rearrange) marked lumps
    **Ctrl-]  Shift-F2**  Sort marked lumps into alphabetical order
    **Ctrl-U  F3      **  Update
    **Ctrl-E  F4      **  Edit
    **Ctrl-O  F5      **  Copy or export lumps; [see below](#copying)
    **        Shift-F5**  Export as raw, no file conversion
    **Ctrl-B  F6      **  Rename selected lump
    **Ctrl-K  F7      **  Make new lump
    **Ctrl-X  F8      **  Delete lump(s)
    **        Shift-F8**  Delete lump(s) (no confirmation)
    **Ctrl-F  F9      **  Export marked lumps to new WAD file; see below
    **Ctrl-A  F10     **  Unmark all marked lumps
    **Ctrl-Z          **  Undo last change
    **Ctrl-Y          **  Redo change

All [standard controls](common.md) are also supported.

## Copying

 * If a WAD file is in the opposite pane, **Copy (F5)** copies the selected lump
   (or tagged lumps) to the other file. The lumps will be inserted into the
   other WAD as new lumps, even if there are already lumps with the same names.
   A horizontal line in the opposite pane shows where the copied lumps will be
   inserted.
 * If a directory is in the opposite pane, **Export (F5)** will export those lumps
   as files into that directory. The lumps will be converted to an appropriate
   file format depending on the type of lump.
   [See the table below for details](#file-formats).
 * Shift-F5 will export to files without performing any conversion (ie. .lmp
   files). You can also this function to export music tracks as .mus files
   instead of converting to .mid.
 * Files can be imported back into WAD files by switching to the opposite pane
   and using **Import (F5)**.
 * If a patch names list is in the opposite pane, **Copy names (F5)** will copy the
   names of the tagged lumps into the list. This is useful if you imported some
   new patches into your WAD and need to add them to PNAMES.
 * **Export as WAD (F9)** will create a new .wad file in the directory in the
   opposite pane. All marked lumps will be copied into the new .wad.

## File formats

Lumps are converted into the following formats when exporting from a WAD (unless
Shift-F5 is used):

    **Lump type                  File extension   File format**
    ---------------------------------------------------------------------------
    DMX MUS music format       .mid             MIDI music track
    Sound effect               .wav             WAVE sound file
    VGA palette lump           playpal.png      Portable Network Graphics
    Color mapping lump         .cmap.png        Portable Network Graphics
    Floor/ceiling texture      .flat.png        Portable Network Graphics
    Graphic lump               .png             Portable Network Graphics
    PNAMES lump                .txt             Plain text, one name per line
    TEXTURE* lump              .txt             Deutex plain text config format
    Hexen full screen graphic  .fullscreen.png  Portable Network Graphics
    STARTUP                    .hires.png       Portable Network Graphics
    Demo (DEMO1-DEMO4)         .lmp             No conversion performed
    Anything else              .lmp             No conversion performed
