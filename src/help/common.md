# Common controls

This help page is a work in progress.

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

## Marking entries

Sometimes it is convenient to perform an operation on multiple files at
once: for example, copying multiple files. This is done by marking multiple
files at once before performing the action. Mark or unmark a file by pressing
**space**; pressing space multiple times will mark multiple files in
sequence. Then press the key associated with the action to perform; for
example, **Ctrl-C** to copy the marked files.

Sometimes it can be tedious to mark each file one by one. When this is the
case it is preferable to use the "mark pattern" command (**Ctrl-G**). This
takes a glob-style wildcard pattern; for example, "*.png" or "*.wav" to
match a set of files by extension, or "SPOS*" to match all lumps in a WAD
with a particular prefix. To select every file, enter "*".

To clear all marks, type **Ctrl-A** or **F10**.

## Using the Mouse

While WadGadget was designed as a keyboard-driven application, you can also use
the mouse. Click on an item to select it, and double-click to open it
(equivalent to pressing the enter key). If your mouse has a scroll wheel, you
can use the wheel to scroll a directory listing up and down (you can do this
without switching the active pane).

Clicking on a keyboard action within the actions list will perform that action.
Again this is equivalent to typing the keyboard shortcut that is shown.

## Searching

TODO

## Commander Mode

TODO
