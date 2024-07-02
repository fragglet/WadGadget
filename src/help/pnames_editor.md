
# PNAMES editor

The PNAMES lump lists all patch names to be used within TEXTURE lumps. If a
patch is not listed in PNAMES, it cannot be used in a texture. The TEXTURE
and PNAMES lumps are therefore closely related. Conceptually the PNAMES lump
is simply a list of lump names, and the PNAMES editor allows this list to
be changed.

The first entry in the window saves all changes and returns to the WAD file
containing the PNAMES lump.

## Keys

    **Ctrl-V  F2      **  Move (rearrange) marked patch names
    **Ctrl-E  F4      **  Edit plain text config file
    **Ctrl-O  F5      **  Copy or export; [see below](#copying)
    **Ctrl-B  F6      **  Rename selected patch name
    **Ctrl-K  F7      **  Add new patch name
    **Ctrl-X  F8      **  Delete patch names(s)
    **        Shift-F8**  Delete patch names(s) (no confirmation)
    **Ctrl-A  F10     **  Unmark all marked patch names
    **Ctrl-Z          **  Undo last change
    **Ctrl-Y          **  Redo change

All [standard controls](common.md) are also supported.

## Copying

 * If another patch names list is in the opposite pane, **Copy names (F5)** will
   add the selected name (or tagged names) into the other list. Duplicate
   names will not be added.
 * Patch names corresponding to lumps can be added to the list by opening the
   same WAD in the opposite pane, selecting the lumps and using **Copy names (F5)**.
 * New patch names are always added to the end of the directory.
 * If a directory is in the opposite pane, **Export config (F5**) will create a
   plain text file in that directory that contains the tagged names, one per
   line. If none are tagged, the file will contain the entire list.
 * To import such a file into the texture directory, select the file in the
   [opposite pane](dir_view.md) and use **Import config (F5)**.
