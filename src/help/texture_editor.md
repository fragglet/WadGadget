# Texture editor

The texture editor screen allows an individual texture to be edited and
changed. It is accessed via the [texture directory](texture_dir.md) (open a TEXTURE1 or
TEXTURE2 lump inside of a WAD file): select a texture to edit and press enter.

Each texture has a width and height, and is composed of one or more patches;
they are layered on top of each other as a kind of collage. Any patches must
be listed in the PNAMES lump; see the [PNAMES editor](pnames_editor.md) for more details.

## Keys

Use the cursor keys to highlight fields and press enter to edit them.

    **        Esc**  Save and exit texture editor
    **Ctrl-T  F2 **  Lower patch in stack
    **Ctrl-U  F3 **  Raise patch in stack
    **Ctrl-K  F7 **  Add new patch
    **Ctrl-X  F8 **  Delete patch

The "lower" and "raise" actions may somewhat unintuitively appear to work
backwards ("raise" moves the patch down in the list, and "lower" moves it up).
It makes more sense when you consider that patches in the list are drawn in
order; those at the end of the list are really the ones on "top" of the stack.
