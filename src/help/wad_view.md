# WAD file editor

[This page shows help about browsing WAD files. For help on the browser
interface in general, [see here](browser.md).]

WAD files ("Where's All the Data?") contain all of the artwork, levels, sound
effects and music used by Doom. Doom mods are usually constructed as "Patch
WAD" (PWAD) files that replace some of the lumps within the main "IWAD" file
(which is usually named doom.wad, doom2.wad or similar depending on the game).
You should usually not be modifying the IWAD file.

The WAD editor allows WAD files to be created and edited. Most importantly it
allows new resources to be [imported](#copying) into the WAD from files, or to be copied
from other WAD files. Each WAD file consists of a number of named "lumps";
they contain different kinds of data depending on their purpose.

It is possible to navigate inside of certain lump types (the [texture directory](texture_dir.md)
and [PNAMES editor](pnames_editor.md)). Selecting the first entry in the WAD list returns to the
directory containing the WAD.

## Keys

    **        Enter   **  View/edit lump
    **Ctrl-D          **  View hex**d**ump of selected lump
    **Ctrl-V  F2      **  Mo**v**e (rearrange) marked lumps
    **Ctrl-]  Shift-F2**  Sort marked lumps into alphabetical order
    **Ctrl-U  F3      **  **U**pdate WAD lumps; [see below](#updating)
    **Ctrl-C  F5      **  **C**opy or export lumps; [see below](#copying)
    **        Shift-F5**  Export as raw, no file conversion
    **Ctrl-E  F6      **  R**e**name selected lump
    **Ctrl-K  F7      **  Ma**k**e new lump
    **Ctrl-X  F8      **  Delete lump(s)
    **        Shift-F8**  Delete lump(s) (no confirmation)
    **Ctrl-F  F9      **  Export marked lumps to new WAD **f**ile; see below
    **Ctrl-A  F10     **  Unmark **a**ll marked lumps
    **Ctrl-Z          **  Undo last change
    **Ctrl-Y          **  Redo change

All [standard controls](browser.md#keys) are also supported.

## Copying

 * If a WAD file is in the opposite pane, **Copy (F5)** copies the selected lump
   (or tagged lumps) to the other file. The lumps will be inserted into the
   other WAD as new lumps, even if there are already lumps with the same
   names.  A horizontal line in the opposite pane shows where the copied lumps
   will be inserted.
 * If a directory is in the opposite pane, **Export (F5)** will export those lumps
   as files into that directory. The lumps will be converted to an appropriate
   file format depending on the type of lump.
   [See the table below for details](#file-formats).
 * **Shift-F5** will export to files without performing any conversion (ie. .lmp
   files). You can also this function to export music tracks as .mus files
   instead of converting to .mid.
 * Files can be imported back into WAD files by switching to the opposite pane
   and using **Import (F5)**.
 * If a patch names list is in the opposite pane, **Copy names (F5)** will copy the
   names of the tagged lumps into the list. This is useful if you imported
   some new patches into your WAD and need to add them to PNAMES.
 * **Export as WAD (F9)** will create a new .wad file in the directory in the
   opposite pane. All marked lumps will be copied into the new .wad.

## Updating

Copying into a WAD (importing) will always add new lumps, even if lumps
already exist with the same name. If your aim is to replace the content of an
existing lump (or lumps), you might instead want to use **Update (F3)**.

**Update** does essentially the same thing as **Import**, except that it will search
for an existing lump with the same name and try to replace its contents. To
use, switch to the other pane and mark the files or lumps you wish to import,
then press **F3**.

There are the following corner cases:

 * Duplicate entries are not tolerated. For example, suppose you are trying to
   import a file named **TITLEPIC.png** and there are two lumps named **TITLEPIC**
   already present in the WAD. The update function will refuse to proceed; it
   will not risk potentially overwriting the wrong lump. You can resolve this
   either by removing the duplicate lumps or by importing manually (use the
   regular **Import** function and delete the old lump).
 * If some of the lumps to update cannot be found, the update function will
   prompt you as to whether to add those as new lumps (the behavior in this
   case is identical to the **Import** function). If you say no, nothing gets
   updated.
 * Level lumps are handled correctly if you are updating a WAD with the
   contents of another WAD. For example, if you select the **LINEDEFS** lump that
   belongs to **MAP08**, it is smart enough to find the matching lump in the WAD
   being updated.

## File formats

Lumps are converted into the following formats when exporting from a WAD
(unless **Shift-F5** is used):

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
