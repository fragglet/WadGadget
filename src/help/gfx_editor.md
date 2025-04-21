# Graphics editor

The graphics editor screen allows the header fields of graphic lumps to be
changed. The primary purpose of this is to allow the X and Y offsets to be
adjusted; these perform different functions depending on the type:

* For Doom's UI element graphics (eg. menu text lumps), the offsets adjust
  the position at which the graphic will be drawn on the screen.
* For sprites (ie. in-game objects), the offsets adjust the centering (X
  offset) and height above the ground (Y offset).

Note that the editor **only** allows the header fields to be changed; the
pixel data itself can be edited with external tools.

## Changing graphic dimensions

The graphics editor also allows the dimensions of the graphic to be changed.
In general this is safe, with the downside that reducing the image size will
leave some junk/unused data inside the lump, since only the header is
changed. The only ill-advised action is to increase the width of the graphic
beyond its original width; doing so will likely corrupt the graphic.
