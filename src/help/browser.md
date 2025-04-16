# Browser interface

The browser is the main interface presented by WadGadget. It provides a
consistent interface for navigating the filesystem, WAD files and lumps, and
is reminiscent of two-pane Orthodox file managers such as Norton Commander.
The following pages provide more specific information:

 * [Directory view](dir_view.md)
 * [PNAMES editor](pnames_editor.md)
 * [Texture editor](texture_dir.md)
 * [WAD view](wad_view.md)
 * [Palette database](palette.md)

## Keys

The following are standard keys that always work in the browser interface:

    **        Tab**   Switch between panes
    **  Shift-Tab**   Swap panes
    **Space      **   [Mark](#marking-entries)
    **Ctrl-G     **   Mark pattern (**g**lob)
    **Ctrl-A  F10**   Unmark **a**ll
    **Ctrl-N     **   Search again (**n**ext search result)
    **Ctrl-W     **   Clear search
    **Ctrl-J     **   Toggle [Commander Mode](#commander-mode)
    **Ctrl-L     **   Redraw screen
    **        Esc**   Quit

The following are *common* keys which work in many places, but are not always
supported:

    **        Enter   **  View/edit object
    **Ctrl-C  F5      **  **C**opy
    **Ctrl-D          **  View hex**d**ump
    **Ctrl-E  F6      **  R**e**name
    **Ctrl-K  F7      **  Add new object
    **Ctrl-X  F8      **  Delete
    **Ctrl-Y          **  Redo change
    **Ctrl-Z          **  Undo last change

## Marking entries

Sometimes it is convenient to perform an operation on multiple files at once:
for example, copying multiple files. This is done by marking multiple files at
once before performing the action. Mark or unmark a file by pressing **space**;
pressing space multiple times will mark multiple files in sequence. Then press
the key associated with the action to perform; for example, **Ctrl-C** to copy the
marked files.

Sometimes it can be tedious to mark each file one by one. When this is the
case it is preferable to use the "mark pattern" command (**Ctrl-G**). This
takes a glob-style wildcard pattern; for example, "*.png" or "*.wav" to match
a set of files by extension, or "SPOS*" to match all lumps in a WAD with a
particular prefix. To select every file, enter "*" as a pattern.

To clear all marks, type **Ctrl-A** or **F10**.

## Using the Mouse

While WadGadget was designed as a keyboard-driven application, you can also
use the mouse:

* Click on an item to select it, and double-click to open it (equivalent to
  pressing the enter key).
* If your mouse has a scroll wheel, you can use the wheel to scroll a
  directory listing up and down (you can do this without switching the active
  pane).
* Clicking on a keyboard action within the actions list will perform that
  action. Again this is equivalent to typing the keyboard shortcut that is
  shown.

If mouse clicks do not work within your terminal, you may need to enable them
in the configuration settings. For XTerm-based terminals, you might need to
set the "allowMouseOps" X resource. to do this, add a line like the following
to your `.Xresources` file (and then re-run xrdb):

    *VT100*allowMouseOps: true

## Searching

TODO

## Commander Mode

WadGadget's default interface is modeled on TiC's NewWadTool, and has a
central pane that lists all of the available actions and key combinations.
"Commander Mode" rearranges the interface to more closely resemble Orthodox
file managers such as Norton Commander or Midnight Commander.

In Commander Mode, the central pane is replaced by a single line at the bottom
of the screen instead. This is more space efficient, but it's likely that some
of actions will be given abbreviated descriptions, and less common ones might
not be shown at all.  If you're familiar enough with the program, it's
possible you won't mind.

Because it's more space efficient, Commander Mode activates automatically if
the screen dimensions are less than 80 columns x 25 pixels. It can be toggled
manually by pressing Ctrl-J. Pressing Ctrl-J repeatedly will cycle through
several different modes:

 * Normal mode, function keys listed (default)
 * Commander Mode, function keys listed
 * Normal mode, Ctrl- accelerators listed
 * Commander Mode, Ctrl- accelerators listed
