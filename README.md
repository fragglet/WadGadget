![WadGadget icon](wadgadget.svg)

WadGadget is a console-based, interactive WAD file editor for Doom engine games
(and others that reuse the WAD format).

![Screenshot of WadGadget](screenshot.png)

The program is a love letter to TiC's [New WAD
Tool](https://doomwiki.org/wiki/New_WAD_Tool), a '90s WAD editing tool that I
have continued to find useful even two decades after its last release.  The
user interface is inspired both by orthodox file management tools like [Norton
Commander](https://en.wikipedia.org/wiki/Norton_Commander) but also by the
[unfinished 1.4
beta](https://doomwiki.org/wiki/New_WAD_Tool#NWT_pro_beta_release) release of
NWT which was going to have a two-pane interface for viewing two WAD files
simultaneously.

A two-pane WAD editing tool has several advantages. Firstly, when editing Doom
WAD files, one develops PWADs as a patch against the original IWAD file. The
result is a merger between the two, and it's helpful to be able to explore this
and see the effect of one on the other. Secondly, WadGadget fully
integrates both the filesystem and WAD views; the actions of importing,
exporting or copying between WADs all take place through a consistent interface.

## Features

WadGadget aims at minimum for feature parity with NWT; [this goal has not yet
been met](https://github.com/fragglet/WadGadget/milestone/1). It also adds new
features and a more logical GUI, but is likely never going to include as many
features as other tools like the excellent
[SLADE](https://slade.mancubus.net/).

The following table gives a brief summary of the current state:

|                                | NWT                  | WadGadget                                   | SLADE                                |
|--------------------------------|----------------------|---------------------------------------------|--------------------------------------|
| Operating System               | DOS                  | Linux, macOS, BSD, other Unixes             | Windows, macOS, Linux                |
| Software License               | [License-free](https://en.wikipedia.org/wiki/License-free_software); v1.3 source is public | GNU GPLv2 | GNU GPLv2 |
| Interface                      | Text UI (80x25)      | ncurses (~any screen size)                  | GUI (wxWidgets)                      |
| Two pane view                  | ✓ (in 1.4 beta)      | ✓                                           | ✓ (multi-tab)                        |
| File formats                   | WAD                  | WAD                                         | WAD, ZIP, PAK, HOG, many others      |
| Basics: Create, Delete, Rename | ✓                    | ✓                                           | ✓                                    |
| Rearrange lumps within WAD     |                      | ✓                                           | ✓                                    |
| Filesystem navigation          | ✓                    | ✓ (fully integrated with WAD view)          | ✓                                    |
| Basic file management          |                      | ✓ (open, copy, delete, rename files)        | ✓ (kind of, via OS file open dialog) |
| Quick search within WAD        | ✓                    | ✓                                           |                                      |
| Open/edit via external editors |                      | ✓                                           |                                      |
| Quick summary of lump contents | ✓ (graphics, demos)  | ✓ (graphics, demos, SFX, PC speaker sounds) | ✓ (almost everything)                |
| WAD clean/compact              | ✓ (via command line) | ✓                                           | ✓                                    |
| Undo                           |                      | ✓ (multi-level, plus redo)                  | ✓ (multi-level, plus redo)           |
| Hexdump view                   | ✓                    | fragglet/WadGadget#7                        | ✓                                    |
| View ENDOOM                    | ✓                    | ✓                                           | ✓                                    |
| Viewing graphics/flats         | ✓                    | ✓ (for terminals that support Sixels)       | ✓                                    |
| Graphics import                | ✓ (GIF, PCX)         | ✓ (PNG)                                     | ✓ (many formats)                     |
| Graphics export                | ✓ (GIF, PCX)         | ✓ (PNG)                                     | ✓ (many formats)                     |
| Edit graphic offsets           | ✓                    | fragglet/WadGadget#11                       | ✓                                    |
| PNG grAb chunk support         |                      | ✓                                           | ✓                                    |
| Audio playback                 | ✓                    | fragglet/WadGadget#13                       | ✓                                    |
| Audio import                   | ✓ (WAV, VOC)         | ✓ (WAV, VOC, FLAC, others)                  | ✓ (many formats)                     |
| Audio export                   | ✓ (WAV, VOC)         | ✓ (WAV)                                     | ✓ (many formats)                     |
| Music playback                 | ✓                    | fragglet/WadGadget#14                       | ✓                                    |
| Music import                   | ✓                    | ✓                                           | ✓ (many formats)                     |
| Music export                   | ✓ (MUS)              | ✓ (converts MUS to MID)                     | ✓ (many formats)                     |
| Texture editor                 | ✓                    | WIP (fragglet/WadGadget#9); can be edited as text as a stopgap | ✓                 |
| PNAMES editor                  | ✓                    | ✓                                           | ✓                                    |
| Online help                    |                      |                                             | ✓ (browser tabs to access help/wiki) |
| View/edit levels               |                      |                                             | ✓                                    |
| Edit ACS scripts               |                      |                                             | ✓                                    |
| Source port features           |                      |                                             | ✓                                    |
| Scripting                      |                      |                                             | ✓ (via Lua)                          |
| Online help                    |                      |                                             | ✓                                    |
| A zillion other features       |                      |                                             | ✓                                    |

## FAQ

**Can I use this under Microsoft Windows?**

There is not yet a native Windows version. You can probably make it
work by using [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux);
I have not yet heard from anyone who has tried this.

**Will you add {my favorite feature here}?**

Firstly, WadGadget is never going to implement every feature found in other
editors like SLADE. Some of the planned features are listed in the table
above. If there is a particular feature that you think is important to
add to the program, [file a feature request](https://github.com/fragglet/WadGadget/issues/new).

**What features will never be implemented?**

The goal of the project is to develop a curses-based Doom WAD editor. This
necessarily means that various features will always be out of scope. Some
examples are:

* It will never include a level editor, or anything else that requires a
  graphical display (like an image editor). It's better to delegate that
  kind of thing to other programs.
* It is unlikely to ever include any kind of GUI or GUI integration
* It is unlikely there will ever be support for other archive formats, like
  `.zip`/`.pk3` (used in some Doom source ports), or formats like `.pak` or
  `.grp` used in other games.

The project also tries to avoid "useless" features -- for example,
configuration options to change the colors of the interface (Commander
Mode is the one such feature that is the exception).
