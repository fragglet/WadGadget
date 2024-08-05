# Palette database

The Doom engine uses an 8-bit palettized image format for all its graphics,
where each pixel is always a color chosen from one of 256 different colors.
Different Doom engine games use different palettes, and some WADs use their
own modified palettes. If you view a graphic with the wrong palette the
image will display but with the wrong colors.

WadGadget ships with the Doom palette built in, but if you are targeting a
different game (Heretic, Hexen, etc.) you will want to change this. The
palette database allows you to store palettes for different games and WADs,
and quickly switch between them.

You can open the palette database by pressing **Ctrl-P** within the browser.
The database appears as a browser location itself. When you are done, select
the "Back to..." item at the top of the list.

## Keys

    **        Enter   **  View/edit palette
    **Ctrl-U  F3      **  Set preferred palette; [see below](#preferred-palette)
    **Ctrl-D  F4      **  Set **d**efault palette
    **Ctrl-C  F5      **  **C**opy or export palette; [see below](#exporting-palettes)
    **Ctrl-E  F6      **  R**e**name selected palette
    **Ctrl-X  F8      **  Delete palette
    **Ctrl-A  F10     **  Unmark **a**ll marked palette

All [standard controls](common.md) are also supported.

## How the palette is selected

When viewing or exporting a graphic from a WAD, the correct palette to use
must be identified. This is done by checking several things in order:

 * First, if a lump named PALPREF exists in the WAD, this is loaded and
   used in the palette.
 * Secondly, if a lump named PLAYPAL exists in the WAD, this is loaded and
   the first palette in the lump is used.
 * Finally, the default palette is used.

## Default palette

The default palette is used if there is no PALPREF or PLAYPAL lump inside
the WAD. If you are making a WAD for Heretic for example, you can change
the default to the Heretic palette. Press **Ctrl-P** to open the palette
database, select the palette to use and press **Ctrl-D** or **F4** to change the
default.

Note that by default WadGadget only includes the Doom palette; to add
palettes from other games, you'll need to import the PLAYPAL lump from the
IWAD file for that game ([see below](#importing-palettes)).

## Preferred palette

If a lump named PALPREF exists in a WAD file, this is always used as the
palette for displaying graphics from that WAD. This allows the palette to
be selected for a WAD without needing to continually change the default
palette.

To set the preferred palette for a WAD, open the palette database in one
pane and the WAD file in the other. Pressing **Ctrl-U** or **F3** in the palette
database will copy that palette to the WAD and create (or update) a PALPREF
lump. The lump can be safely deleted if no longer needed.

The format of PALPREF is the same as format for the PLAYPAL lump used by
the game itself, but it only contains a single palette (PLAYPAL usually
contains multiple palettes).

## Importing palettes

WadGadget only ships with a single palette, the Doom palette. If you're
working with another game you'll need to import the palette from that game
into the palette database. There are two options:

 * **Import from WAD file**: First, press **Ctrl-P** to open the palette
   database. Switch to the other pane and navigate to the IWAD file for the
   game you're working with (usually named *heretic.wad*, *hexen.wad* or
   similar). Type PLAYPAL to jump to the main palette lump. Press **Ctrl-C** or
   F5 to copy the palette into the database. The palette will have a name
   of the form "heretic.wad palette"; you may want to rename this.

 * **Import from PNG file**: To do this you will need a PNG file containing the
   palette colors; for example, by exporting the PLAYPAL lump from the WAD
   as a PNG file. The number of pixels in the image must be a multiple of
   256. To import it into the palette database, first press **Ctrl-P** to open
   the palette database. Switch to the other pane and navigate to the PNG
   file you want to import. Press **Ctrl-C** or **F5** to import it into the
   palette database.

## Exporting palettes

You can export a palette from the database in two ways:

 * **To a WAD file**: This creates a PLAYPAL lump in the WAD file. Open the
   WAD in which you want to create the lump, then switch to the other pane
   and press **Ctrl-P** to open the palette database. Select the palette to
   copy and press **Ctrl-C** or **F5** to copy to the WAD; the new lump will be
   named PLAYPAL.

   There is a "gotcha" here to be aware of. A PLAYPAL lump contains
   multiple palettes (used in game for when the player suffers damage,
   picks up items, etc.), and the default palette shipped with the program
   only contains the normal Doom palette. You cannot therefore use it to
   create a PLAYPAL lump. You will instead need to import the full PLAYPAL
   lump from the Doom IWAD file that contains all the other palettes.

 * **To a PNG file**: Open the directory to copy the palette into. Switch to
   the other pane and press **Ctrl-C** or **F5** to export the palette to a PNG
   file.
