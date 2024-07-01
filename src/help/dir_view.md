# Directory view

The directory view shows a particular directory (folder) on the filesystem.
Selecting a directory and pressing enter will navigate into that directory;
doing this for the first entry in the list will navigate to the parent
directory. Selecting a WAD file and pressing enter will open that WAD file for
editing (the [WAD view](wad_view.md)).

Selecting a normal file will either view the contents of the file (for text
files or for .png graphic files), or open the file in the system's default
editor for that file type.

## Keys

            Enter     View/edit file
    Ctrl-D            View hexdump of selected file
    Ctrl-P  F2        Compact selected WAD file
    Ctrl-U  F3        Update
    Ctrl-E  F4        Edit
    Ctrl-O  F5        Copy or import files; [see below](#copying)
            Shift-F5  Import raw, no file conversion
    Ctrl-B  F6        Rename selected file/directory
    Ctrl-K  F7        Make directory
    Ctrl-X  F8        Delete file(s)
            Shift-F8  Delete file(s) (no confirmation)
    Ctrl-F  F9        Make WAD containing marked files; see below
            Shift-F9  Make WAD, no file conversion
    Ctrl-A  F10       Unmark all marked files
    Ctrl-R            Reload directory

All [standard controls](common.md) are also supported.

## Copying

 * If a WAD file is in the opposite pane, Import (F5) imports the selected file
   (or tagged files) into the WAD. The files will be automatically converted
   into the WAD native format based on the file's name and extension;
   ([see the table below](#file-formats)). New lumps are always created, even if
   there are already other lumps with the same names. The horizontal line in the
   opposite pane shows where they will be inserted.
 * Use Shift-F5 to import into a WAD file without performing any file format
   conversion (ie. ignore filename, import files as raw lumps).
 * If another directory is in the opposite pane, Copy (F5) performs a normal
   file copy. Mark multiple files to copy multiple files, otherwise the
   currently selected file is copied. You cannot copy directories.
 * If the same directory is in the opposite pane, Copy (F5) will make a duplicate
   copy of the selected file; you will be prompted to enter a name for the new
   file. You cannot do this with multiple files at the same time.
 * If a texture directory is in the opposite pane, Import config (F5) will parse
   the selected file as a deutex plain text texture file, and then insert the
   textures into the texture directory. Existing textures with the same names
   will be overwritten; any new textures will be inserted at the position
   indicated by a horizontal line in the opposite pane.
 * Make WAD (F9) creates a new .wad file in the same directory and copies the
   marked files into the .wad file. If no files are marked, an empty .wad file
   is created.

## File formats

File formats when importing to a WAD (unless Shift-F5 is used):

    Extension        Lump type                  Notes
    ---------------------------------------------------------------------------
    .lmp             Raw lump or demo file      No conversion performed
    .mus             DMX MUS music track        No conversion performed
    .mid             MIDI music track           No conversion performed
    .wav             Sound effect
    .voc             Sound effect
    .flac            Sound effect
    playpal.png      VGA palette lump           Must be multiple of 256 pixels
    colormap.png     Color mapping lump         Must be multiple of 256 pixels
    .cmap.png        Color mapping lump         Must be multiple of 256 pixels
    .flat.png        Floor/ceiling texture      Must be 64 pixels wide
    .png             Graphic lump
    PNAMES.txt       Plain text patch names
    TEXTURE*.txt     Plain text texture config  Must already have a PNAMES lump
    .fullscreen.png  Hexen full screen image    Must be 320x200 pixels
