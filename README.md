WadGadget is a love letter to TiC's [New WAD
Tool](https://doomwiki.org/wiki/New_WAD_Tool), a '90s WAD editing tool
that I continue to find useful even two decades after its last release.

![Screenshot of WadGadget](screenshot.png)

The user interface is inspired both by orthodox file management tools like
[Norton Commander](https://en.wikipedia.org/wiki/Norton_Commander) but also
by the [unfinished 1.4
beta](https://doomwiki.org/wiki/New_WAD_Tool#NWT_pro_beta_release) release
of NWT which was going to have a two-pane interface for viewing two WAD
files simultaneously. The advantage of this approach in a WAD editing tool
is simple: when editing Doom WAD files, one develops PWADs as a patch
against the original IWAD file. The result is a merger between the two, and
it's helpful to be able to explore this and see the effect of one on the
other.

## Features

WadGadget aims at minimum for feature parity with NWT; this goal has not
yet been met. It also adds new features and a more logical GUI, but is
likely never going to include as many features as other tools like
the excellent [SLADE](https://slade.mancubus.net/).

The following table gives a brief summary of the current state:

|                                | NWT                  | WadGadget                                   | SLADE                                |
|--------------------------------|----------------------|---------------------------------------------|--------------------------------------|
| Operating System               | DOS                  | Linux, BSD, other Unixes                    | Windows, macOS, Linux                |
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
| Audio playback                 | ✓                    |                                             | ✓                                    |
| Audio import                   | ✓ (WAV, VOC)         | ✓ (WAV, VOC, FLAC, others)                  | ✓ (many formats)                     |
| Audio export                   | ✓ (WAV, VOC)         | ✓ (WAV)                                     | ✓ (many formats)                     |
| Music playback                 | ✓                    |                                             | ✓                                    |
| Music import                   | ✓                    | ✓                                           | ✓ (many formats)                     |
| Music export                   | ✓ (MUS)              | ✓ (converts MUS to MID)                     | ✓ (many formats)                     |
| Texture editor                 | ✓                    | fragglet/WadGadget#9                        | ✓                                    |
| PNAMES editor                  | ✓                    | fragglet/WadGadget#10                       | ✓                                    |
| Online help                    |                      |                                             | ✓ (browser tabs to access help/wiki) |
| View/edit levels               |                      |                                             | ✓                                    |
| Edit ACS scripts               |                      |                                             | ✓                                    |
| Source port features           |                      |                                             | ✓                                    |
| Scripting                      |                      |                                             | ✓ (via Lua)                          |
| Online help                    |                      |                                             | ✓                                    |
| A zillion other features       |                      |                                             | ✓                                    |
