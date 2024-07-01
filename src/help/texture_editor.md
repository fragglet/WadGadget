# Texture editor

The TEXTURE lump defines all of the wall textures that are used within the game.
These are the textures that are specified on sidedefs when editing a level. A
texture has a width and height, and is composed of one or more patches; they
are layered on top of each other as a kind of collage. Any patches must be
listed in the PNAMES lump; see the [PNAMES editor](pnames_editor.md) for more
details.

The first entry in the window saves all changes and returns to the WAD file
containing the texture lump.

## Keys

Ctrl-V  F2        Move (rearrange) marked textures
Ctrl-]  Shift-F2  Sort marked textures into alphabetical order
Ctrl-U  F3        Duplicate selected texture
Ctrl-E  F4        Edit texture config
Ctrl-O  F5        Copy or export; [see below](#copying)
Ctrl-B  F6        Rename selected texture
Ctrl-K  F7        Make new texture
Ctrl-X  F8        Delete texture(s)
        Shift-F8  Delete texture(s) (no confirmation)
Ctrl-A  F10       Unmark all marked textures
Ctrl-Z            Undo last change
Ctrl-Y            Redo change

All [standard controls](common.md) are also supported.

## Copying

 * If another texture directory is in the opposite pane, Copy (F5) copies the
   selected texture (or marked textures) to the other directory. This can be
   used to copy texture definitions between WADs. If textures with the same
   names already exist, they will be overwritten; any new textures will be
   inserted into the other directory at the position indicated by a horizontal
   line in the opposite pane.
 * If a directory is in the opposite pane, Export config (F5) will create a
   plain text file in that directory, containing the tagged textures in the
   deutex plain text format. If no textures are tagged, the entire directory is
   exported.
 * To import such a text file back into the texture directory, switch to the
   [opposite pane](directory_view.md) and use Import config (F5).
 * Texture directories go hand-in-hand with PNAMES lumps. If you add textures
   into a directory that use new PNAMES, it is important that you update the
   PNAMES lump when prompted.

## deutex texture configuration format

TODO
