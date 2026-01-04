# Hexdump view

The hexdump view allows the raw bytes of files and lumps to be viewed.
This is usually only useful to advanced users.

## Keys

    **Esc       q**  Close hexdump view
    **Up/Down    **  Scroll up/down through file
    **PgUp/PgDn  **  Scroll up/down whole page
    **Home/End   **  Jump to beginning/end of file
    **Ctrl-B     **  Change columns/bytes per line - ([see below](#columns))
    **Ctrl-D     **  Switch to ASCII (plain text) view
    **Ctrl-F    /**  Search for text
    **Ctrl-N    n**  Next search result
    **Ctrl-O     **  Open Doom specs ([see below](#consulting-the-specs))
    **Ctrl-R     **  Change record length ([see below](#record-grouping))

## Columns

By default the hexdump viewer shows 16 bytes per line (it may be smaller if
your screen width is too small to show this much). This can be changed by
pressing **Ctrl-B**.

## Record grouping

A number of lumps within the WAD format take the form of an array of records,
each of which has a fixed length. An example is that the **THINGS** lump consists
of 10 byte records. When looking at lumps like these, it is very useful to be
able to see where each record starts. Pressing **Ctrl-R** allows a record
length to be set; when set, the first byte of each record is highlighted in
**bold** text. Setting an empty record length will clear it.

The hexdump view automatically identifies the record length for some common
lump types:

 * PLAYPAL (one per palette)
 * COLORMAP (one per colormap)
 * ENDOOM (one per line)
 * THINGS
 * LINEDEFS
 * SIDEDEFS
 * SECTORS
 * VERTEXES
 * SSECTORS
 * NODES
 * SEGS

Some Doom engine games use different record lengths; for example, the record
length for **LINEDEFS** lumps is different for Hexen format levels. In this case
the record length might need to be manually overridden to a different value.

In addition to seeing where records begin, it is also useful to have the
records vertically aligned. The hexdump view will therefore automatically
set the display columns to be a multiple of the record length - or at least
a factor of it if an entire record cannot fit on a single line.

## Consulting the specs

Pressing **Ctrl-O** opens the help system to view the [Unofficial Doom Specs](uds.md).
These include a lot of information about the format of the lumps found in
Doom WADs, and can help you interpret the lump data you're looking at. If the
type of the lump you are viewing is recognized, you may be immediately shown
the section documenting it. For some lump types you may be shown a section
from one of the following specs files instead:

 * [Official Hexen Specs](hexen_specs.md)
 * [Boom Reference](boomref.md)
 * [MBF reference](mbfedit.md)

Pressing **Escape** once will go back to the hexdump view, leaving the specs
open in the background so that you can continue to consult them. Press **Escape**
a second time to close the specs window. Pressing **Ctrl-O** will re-open the specs
at the same location you were previously viewing.
