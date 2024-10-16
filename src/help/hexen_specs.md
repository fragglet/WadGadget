# Official Hexen Technical Specs

                 The Official Hexen Technical Specs
             Author: Ben Morris (bmorris@islandnet.com)
            Information from Raven provided by Ben Gokey

# Disclaimer

The text contained in this document is for informational purposes only.
If you decide to use this information in any way, neither id Software,
Raven Software, nor Ben Morris can be held responsible for any damages
or losses (including, but not limited to: dismembered bodily parts,
telefrags and lack of sleep) incurred by this information's use.
Although this is an "Official" specification, some of the information
contained within might be old, or just plain typed in wrong.  You have
been warned.

NB: This version of the specs, 0.9, is a preliminary release. Most of
the information here is tried and true, but there's a good chance there
are errors in the file.  If something doesn't look right, or really IS
wrong, please contact me (Ben Morris) at the address above.  Please do
NOT contact me about new versions of the specs; I will release the new
versions when they are ready.  Thanks.

# Table of Contents

 * [[0]](#0-definitions-used-in-this-file) Definitions used in this File
 * [[1]](#1-about-this-file) About This File
 * [[2]](#2-introduction-to-hexen) Introduction to Hexen
 * [[3]](#3-hexen-data-structures) Hexen Data Structures
    * [[3-1]](#3-1-the-hexen-linedef-structure) The Hexen LINEDEF structure
      * [[3-1-1]](#3-1-1-line-flags) Line Flags
    * [[3-2]](#3-2-the-hexen-thing-structure) The Hexen THING structure
      * [[3-2-1]](#3-2-1-thing-flags) Thing Flags
      * [[3-2-2]](#3-2-2-thing-types) Thing Types
 * [[4]](#4-hexen-script-language) Hexen Script Language
    * [[4-1]](#4-1-variables-and-their-scope) Variables and their Scope
    * [[4-2]](#4-2-language-structure) Language Structure
      * [[4-2-1]](#4-2-1-keywords) Keywords
      * [[4-2-2]](#4-2-2-comments) Comments
      * [[4-2-3]](#4-2-3-world-variable-definitions) World-variable definitions
      * [[4-2-4]](#4-2-4-map-variable-definitions) Map-variable definitions
      * [[4-2-5]](#4-2-5-define-directive) Include Directive
      * [[4-2-5]](#4-2-5-define-directive) Define Directive
      * [[4-2-6]](#4-2-6-statements) Constant Expressions
      * [[4-2-7]](#4-2-7-internal-functions) String Literals
      * [[4-2-6]](#4-2-6-statements) Script Definitions
      * [[4-2-6]](#4-2-6-statements) Statements
        * [[4-2-6-1]](#4-2-6-1-assignment-statements) Declaration Statements
        * [[4-2-6-1]](#4-2-6-1-assignment-statements) Assignment Statements
        * [[4-2-6-2]](#4-2-6-2-compound-statements) Compound Statements
        * [[4-2-6-3]](#4-2-6-3-switch-statements) Switch Statements
        * [[4-2-6-4]](#4-2-6-4-jump-statements) Jump Statements
        * [[4-2-6-5]](#4-2-6-5-iteration-statements) Iteration Statements
        * [[4-2-6-6]](#4-2-6-6-function-statements) Function Statements
        * [[4-2-6-7]](#4-2-6-7-print-statements) Print Statements
        * [[4-2-6-8]](#4-2-6-8-selection-statements) Selection Statements
        * [[4-2-6-9]](#4-2-6-9-control-statements) Control Statements
      * [[4-2-7]](#4-2-7-internal-functions) Internal Functions
 * [[5]](#5-flats-with-special-properties) Flats with special properties
 * [[6]](#6-the-mapinfo-lump) The MAPINFO lump
 * [[7]](#7-polyobjects) PolyObjects
    * [[7-1]](#7-1-polyobj-start-spots-and-anchor-points) Polyobj Start Spots and Anchor Points

APPENDICES

 * [[A]](#appendix-a-list-of-spawnable-objects): List of Spawnable Objects
 * [[B]](#appendix-b-list-of-activateabledeactivateable-objects): List of Activateable/Deactivateable Objects
 * [[C]](#appendix-c-list-of-things-that-require-arguments): List of THINGS that require arguments
 * [[D]](#appendix-d-sector-specials): Sector Specials
 * [[E]](#appendix-e-action-specials): Action Specials
    * [[E-1]](#e-1-floor-and-ceiling-specials) Floor and Ceiling Specials
    * [[E-2]](#e-2-stair-specials) Stair Specials
    * [[E-3]](#e-3-door-specials) Door Specials
    * [[E-4]](#e-4-script-specials) Script Specials
    * [[E-5]](#e-5-light-specials) Light Specials
    * [[E-6]](#e-6-miscellaneous-specials) Miscellaneous Specials
    * [[E-7]](#e-7-thing-specials) Thing Specials
    * [[E-8]](#e-8-polyobject-specials) PolyObject Specials
 * [[F]](#appendix-f-sector-sounds-for-changesectorsound-special): Sector Sounds for ChangeSectorSound() special
 * [[G]](#appendix-g-key-numbers): Key Numbers

# [0] Definitions used in this File

    Angle [0..255]  Used in "angle" parameters to Special types:

                    0       East            32      Northeast
                    64      North           96      Northwest
                    128     West            160     Southwest
                    192     South           224     Southeast

 * NOTE that this differs from DOOM/Heretic in that 45/90
   degree increments are not used.  However, this difference
   does _not_ apply for the angles used for the THINGS in
   map editing - they are the same as DOOM's (eg: 0 = East,
   90 = North, etc.)

    Tics            Time unit of length 1/35 second.  So, 35 tics = 1 second.

    Octics          Time unit of length 8 tics.  So, 8 octics = 1 second.

# [1] About This File

This file was written for those who are interested in the inner workings of
Hexen.  It doesn't contain playing tips or information on how to get Hexen
working on your system.

This file is intended to be supplementary to Matt Fell's
"[Unofficial DOOM Specs](uds.md)", which probably came with your copy of DOOM or
DOOM ][. Wherever it's relevant, this file refers to a specific section in the
DOOM specs (be sure you have version 1.666!), so it's a good idea to have a
copy at hand.

# [2] Introduction to Hexen

Hexen is the sequel to Heretic, Raven Software's first collaboration with id
Software.

[..]

Hexen's major difference from Heretic and DOOM is its programmability.  Hexen
features a powerful script language that can be used to create a wide variety
of in-game effects such as traps, puzzles and even earthquakes!

[...]

# [3] Hexen Data Structures

This section outlines the format of the new data blocks in a Hexen map - the
LINEDEF and THING structures.  These structures have changed from the
versions used in DOOM and Heretic.

[DOOM Specs ref]

## [3-1] The Hexen LINEDEF structure

    Offset      Size        Meaning
    ---------------------------------------------------
    0           word        the line's start-vertex
    2           word        the line's end-vertex
    4           word        line flags (see below)
    6           byte        special type (see [Specials])
    7         5 bytes       special arguments
    12          word        the line's right sidedef number
    14          word        the line's left sidedef number

## [3-1-1] Line Flags

The following flags are starred with an asterisk if they're new for
Hexen:

      Bits        Meaning when Set
      ---------------------------------------------------
      0           impassable - the line cannot be crossed.
      1           impassable to monsters only.
      2           two-sided
      3           upper texture is unpegged (drawn from top-down)
      4           lower/middle texture is unpegged (drawn from bottom-up)
      5           secret - the line appears as impassable on the automap.
      6           sound can't travel through the line, but only
                  for monsters' ears.
      7           never draw the line on the auto-map, even with the
                  map cheat enabled.
      8           the line is always drawn on the auto-map, even if it
                  hasn't been seen by the player.

    * 9           the line's special ([[3-1]](#3-1-the-hexen-linedef-structure)) is repeatable, ie: it can
                  be activated more than once.
    * 10..12      the line's special activation, ie: how the special is
                  activated.

                  Value   Activated when...
                  ---------------------------------------------
                  0       Player crosses the line
                  1       Player uses the line with the use key
                  2       Monster crosses the line
                  3       Projectile impacts the wall
                  4       Player pushes the wall
                  5       Projectile crosses the line

                  To get the special activation, use the following formula:

                  activation := (line.flags BITAND 0x1C00) BITSHIFTRIGHT 10

## [3-2] The Hexen THING structure

      Offset      Size        Meaning
      ---------------------------------------------------
    * 0           word        thing ID - used in scripts and specials to
                              identify a THING or a set of THINGs.
      2           word        x-position on the map
      4           word        y-position on the map
    * 6           word        starting altitude on the map - the THING
                              is created at this altitude above the floor
                              of the sector it's in when the map is entered,
                              and is immediately subjected to gravity.
      8           word        the angle the thing is facing when the map
                              is entered.
      10          word        the thing type (see [[3-2-2]](#3-2-2-thing-types).)
    * 12          word        thing flags (see [[3-2-1]](#3-2-1-thing-flags).)
      14          byte        special type (see [Specials]).  a thing's
                              special is activated when the thing is
                              killed (Monster), destroyed (Tree, Urn, etc.),
                              or picked up (Artifact, Puzzle Piece.)
      15        5 bytes       special arguments

## [3-2-1] Thing Flags

The following flags are starred with an asterisk if they're new for
Hexen:

      Bits        Meaning when Set
      ---------------------------------------------------
      0           the thing appears on the Easy skill settings (1-2)
      1           the thing appears on the Normal skill setting (3)
      2           the thing appears on the Hard skill settings (4-5)
      3           the thing is deaf - it sits around until it's
                  hurt, or until it sees a player.
    * 4           the thing is dormant - it never wakes up until it's
                  activated using the Thing_Activate() special.
    * 5           the thing appears for the Fighter class.
    * 6           the thing appears for the Cleric class.
    * 7           the thing appears for the Mage class.
    * 8           the thing appears in single-player games.
    * 9           the thing appears in cooperative games.
    * 10          the thing appears in deathmatch games.

Each "thing appears" flag must be set for each condition under which the
thing is to appear.  For multi-player games involving more than one
class, a thing that is set for one of the classes involved will also appear
for the other two classes in the game.

For example, if you set the three pieces of the Fighter's sword to appear
for only the Fighter (bit 5 is set) and in Deathmatch (bit 10 is set),
if a Mage or a Cleric is also playing, the pieces of the sword will be
visible to them, too.

## [3-2-2] Thing Types

Creatures as well as some objects can be activated and/or deactivated with
the ThingActivate and ThingDeactivate line specials.

Creatures will freeze when deactivated and resume when activated.
Activation can also be used to bring a "dormant" creature to life.

If a creature has a special, that special will be activated upon its death.
Also, if the creature is teleported away using the banishment device
(teleport other), the special will be activated and then removed from the
creature.

    Type      Name
    -----------------------------------------------------------
    1         Player_1_start
    2         Player_2_start
    3         Player_3_start
    4         Player_4_start
    11        Player_Deathmatch
    14        Player_TeleportSpot

    10        2C_SerpentStaff
    8010      2F_Axe
    53        2M_ConeOfShards

    8009      3C_Firestorm
    123       3F_Hammer
    8040      3M_Lightning
    20        4C_1Shaft
    19        4C_2Cross
    18        4C_3Arc
    16        4F_1Hilt
    13        4F_2Crosspiece
    12        4F_3Blade
    23        4M_1Stick
    22        4M_2Stub
    21        4M_3Skull

    10110     A_Flechette
    8003      A_BoostMana
    8002      A_BootsOfSpeed
    8041      A_Bracers
    84        A_IconOfDefender
    30        A_Porkelator
    83        A_WingsOfWrath
    32        A_HealingComplete (Urn)
    82        A_HealingHefty (Flask)
    81        A_HealingWimpy (Vial)
    10120     A_HealRadius
    8000      A_Repulsion
    86        A_DarkServant
    36        A_ChaosDevice
    33        A_Torch
    10040     A_Banishment

    8008      Ar_Amulet
    8005      Ar_Armor
    8007      Ar_Helmet
    8006      Ar_Shield

    114       C_Bishop
    107       C_Centaur
    115       C_CentaurLeader
    10101     C_ClericBoss
    31        C_Demon
    8080      C_Demon2
    254       C_Dragon
    10030     C_Ettin
    10100     C_FighterBoss
    10060     C_FireImp
    112       C_Fly
    10080     C_Heresiarch
    8020      C_IceGuy
    10200     C_Korax
    10102     C_MageBoss

    121       C_Serpent
    120       C_SerpentLeader
    34        C_Wraith
    10011     C_Wraith2

    8032      K_AxeKey
    8034      K_CastleKey
    8031      K_CaveKey
    8035      K_DungeonKey
    8033      K_FireKey
    8200      K_GoldKey
    8037      K_RustyKey
    8036      K_SilverKey
    8030      K_SteelKey
    8039      K_SwampKey
    8038      K_WasteKey

    122       Mana_1
    124       Mana_2
    8004      ManaCombined

    3000      PO_Anchor
    3001      PO_StartSpot
    3002      PO_StartSpot_Crush

    1410      SE_Wind

    10225     Spawn_Bat
    10000     Spawn_Fog
    10001     Spawn_Fog_a
    10002     Spawn_Fog_b
    10003     Spawn_Fog_c
    113       Spawn_Leaf

    10090     Spike_Down
    10091     Spike_Up

    1403      SS_Creak
    1408      SS_EarthCrack
    1401      SS_Heavy
    1407      SS_Ice
    1405      SS_Lava
    1402      SS_Metal
    1409      SS_Metal2
    1404      SS_Silent
    1400      SS_Stone
    1406      SS_Water

    9001      X_MapSpot
    9013      X_MapSpotGravity

    8064      Z_ArmorSuit
    77        Z_Banner
    8100      Z_Barrel
    8065      Z_Bell
    8066      Z_BlueCandle
    8061      Z_BrassBrazier
    8103      Z_Bucket
    119       Z_Candle
    8069      Z_Cauldron
    8070      Z_Cauldron_Unlit
    8071      Z_Chain32
    8072      Z_Chain64
    8073      Z_ChainHeart
    8074      Z_ChainLHook
    8075      Z_ChainSHook
    8077      Z_ChainSkull
    8076      Z_ChainSpikeBall
    17        Z_Chandelier
    8063      Z_Chandelier_Unlit
    8042      Z_FireBull
    8043      Z_FireBull_Unlit
    8060      Z_FireSkull
    118       Z_GlitterBridge
    10503     Z_LargeFlame_Permanent
    10502     Z_LargeFlame_Timed
    10501     Z_SmallFlame_Permanent
    10500     Z_SmallFlame_Timed
    140       Z_TeleportSmoke
    116       Z_TwinedTorch
    117       Z_TwinedTorch_Unlit
    103       Z_VasePillar
    54        Z_Wall_Torch_Lit
    55        Z_Wall_Torch_Unlit
    5         Z_WingedStatue
    6         ZC_Rock1
    7         ZC_Rock2
    9         ZC_Rock3
    15        ZC_Rock4
    41        ZC_ShroomLarge
    42        ZC_ShroomSmall1
    44        ZC_ShroomSmall2
    45        ZC_ShroomSmall3
    52        ZC_StalactiteLarge
    56        ZC_StalactiteMedium
    57        ZC_StalactiteSmall
    48        ZC_Stalagmite_Pillar
    49        ZC_StalagmiteLarge
    50        ZC_StalagmiteMedium
    51        ZC_StalagmiteSmall
    8062      ZF_DestructibleTree
    8068      ZF_Hedge
    8104      ZF_ShroomBoom
    39        ZF_ShroomLarge1
    40        ZF_ShroomLarge2
    46        ZF_ShroomSmall1
    47        ZF_ShroomSmall2
    8101      ZF_Shrub1
    8102      ZF_Shrub2
    29        ZF_StumpBare
    28        ZF_StumpBurned
    24        ZF_TreeDead
    25        ZF_TreeDestructible
    80        ZF_TreeGnarled1
    87        ZF_TreeGnarled2
    78        ZF_TreeLarge1
    79        ZF_TreeLarge2
    111       ZG_BloodPool
    71        ZG_CorpseHanging
    61        ZG_CorpseKabob
    108       ZG_CorpseLynched
    109       ZG_CorpseNoHeart
    110       ZG_CorpseSitting
    62        ZG_CorpseSleeping
    8067      ZG_IronMaiden
    65        ZG_TombstoneBigCross
    69        ZG_TombstoneBrianP
    66        ZG_TombstoneBrianR
    67        ZG_TombstoneCrossCircle
    63        ZG_TombstoneRIP
    64        ZG_TombstoneShane
    68        ZG_TombstoneSmallCross
    93        ZI_IceSpikeLarge
    94        ZI_IceSpikeMedium
    95        ZI_IceSpikeSmall
    89        ZI_IcicleLarge
    90        ZI_IcicleMedium
    91        ZI_IcicleSmall
    8502      ZM_CandleWeb
    8509      ZM_CleaverMeat
    8508      ZM_GobletSilver
    8507      ZM_GobletSmall
    8505      ZM_GobletSpill
    8506      ZM_GobletTall
    8504      ZM_LgCandle
    8500      ZM_LgStein
    104       ZM_Pot1
    105       ZM_Pot2
    106       ZM_Pot3
    100       ZM_Rubble1
    101       ZM_Rubble2
    102       ZM_Rubble3
    8503      ZM_SmCandle
    8501      ZM_SmStein
    8051      ZP_GargBrnzShort
    8047      ZP_GargBrnzTall
    8044      ZP_GargCorrode
    76        ZP_GargIceShort
    73        ZP_GargIceTall
    8050      ZP_GargLavaBrtShort
    8046      ZP_GargLavaBrtTall
    8049      ZP_GargLavaDrkShort
    8045      ZP_GargLavaDrkTall
    74        ZP_GargPortalShort
    72        ZP_GargPortalTall
    8052      ZP_GargStlShort
    8048      ZP_GargStlTall
    88        ZS_Log
    58        ZS_Moss1
    59        ZS_Moss2
    37        ZS_Stump1
    38        ZS_Stump2
    27        ZS_Tree1
    26        ZS_Tree2
    60        ZS_Vine
    99        ZW_RockBlack
    97        ZW_RockBrownLarge
    98        ZW_RockBrownSmall
    9003      ZZ_BigGem
    9007      ZZ_Book1
    9008      ZZ_Book2
    9016      ZZ_CWeapon
    9015      ZZ_FWeapon
    9018      ZZ_Gear
    9019      ZZ_Gear2
    9020      ZZ_Gear3
    9021      ZZ_Gear4
    9006      ZZ_GemBlue1
    9010      ZZ_GemBlue2
    9005      ZZ_GemGreen1
    9009      ZZ_GemGreen2
    9012      ZZ_GemPedestal
    9004      ZZ_GemRed
    9017      ZZ_MWeapon
    9002      ZZ_Skull
    9014      ZZ_Skull2
    9011      ZZ_WingedStatueNoSkull

# [4] Hexen Script Language

The Hexen Script Language is called the "Action Code Script", or ACS.

Each map has an ACS file that contains the scripts specific to that map. The
scripts within it are identified using numbers that the general special
ACS_Execute() uses.  A script itself can call the ACS_Execute() special
(actually quite common), which will spawn another script that will run
concurrently with the rest of the scripts.  A script can also be declared as
OPEN, which will make it run automatically upon entering the map.  This is
used for perpetual type effects, level initialization, etc.  The compiler
takes the ACS file and produces and object file that is the last lump in the
map WAD (BEHAVIOR).

To create a compiled ACS file from a text script, use the DOS command:

    c:\hexen> acs filename [enter]

This command will produce 'filename.o' from 'filename.acs'.  The contents of
the output file (filename.o) can be directly used as the BEHAVIOR lump of the
map it's used with.

Map scripts should start with #include "common.acs", which is just...

    #include "specials.acs"
    #include "defs.acs"
    #include "wvars.acs"

The file "specials.acs" defines all the general specials.  These are used
within scripts just like function calls.  The file "defs.acs" defines a
bunch of constants that are used by the scripts.  The file "wvars.acs"
defines all the world variables.  It needs to be included by all maps so
they use consistent indexing.

## [4-1] Variables and their Scope

There is only one data type ACS, a 4 byte integer.  Use the keyword int to
declare an integer variable.  You may also use the keyword str, it is
synonymous with int.  It's used to indicate that you'll be using the
variable as a string.  The compiler doesn't use string pointers, it uses
string handles, which are just integers.

Declaring a variable is simple.  There are two "types" of variable - "str"
and "int":

    str mystring;
    int myint;

or:

    str texture, sound;
    int i, tid;

* Note: You can't assign a variable in its declaration; you must give it a
  value in a different expression.

The SCOPE of a variable is one of World-scope, Map-scope, or Script-scope.

 - World-scope variables are global, and can be accessed in any map.
   Hexen maintains [n] permanent globals, numbered 0-[n-1].  You must
   assign one of the globals a name in order to access it, like this:

    world int 5:Grunt;

   This tells Hexen to reference world global number 5 whenever it
   encounters the name "Grunt".

 - Map-scope variables are local to the current map.  They must be
   declared outside of any script code, but without the world keyword.
   These variables can't be accessed in any other map.

 - Script-scope variables are local to the current script - they
   can't be accessed by any other script or map.

Here's some code that shows the declaration of all three scopes:

    world int 3:DungeonAccess; // World-scope

    int mapTimer; // Map-scope

    script 4 (void)
    {
        int x, y; // Script-scope

        ...
    }

## [4-2] Language Structure

Here is a quick reference manual type definition of the language.  It
ends with a description of all the internal functions.

### [4-2-1] Keywords

The following identifiers are reserved for use as keywords, and may
not be used otherwise:

    break
    case
    const
    continue
    default
    define
    do
    else
    goto
    if
    include
    int
    open
    print
    printbold
    restart
    script
    special
    str
    suspend
    switch
    terminate
    until
    void
    while
    world

### [4-2-2] Comments

Comments are ignored by the script compiler.

    /*
       This is a comment.
    */

    int a; // And this is a comment

### [4-2-3] World-variable definitions

    world int <constant-expression> : <identifier> ;
    world int <constant-expression> : <identifier> , ... ;

### [4-2-4] Map-variable definitions

Declares a variable local to the current map.

    int <identifier> ;
    str <identifier> ;
    int <identifier> , ... ;

### [4-2-5] Include Directive

Includes the source of the specified file and compiles it.

    #include <string-literal>

### [4-2-5] Define Directive

Replaces an identifier with a constant expression.

    #define <identifier> <constant-expression>

### [4-2-6] Constant Expressions

    <integer-constant>:

    decimal      200
    hexadecimal  0x00a0, 0x00A0
    fixed point  32.0, 0.5, 103.329
    any radix    <radix>_digits

    binary        2_01001010
    octal         8_072310
    decimal       10_50025
    hexadecimal   16_00a03f2

### [4-2-7] String Literals

    <string-literal>: "string"

### [4-2-6] Script Definitions

To define a script:

    <script-definition>:
      script <constant-expression> ( <arglist> ) { <statement> }
      script <constant-expression> OPEN { <statement> }

eg:

    script 10 (void) { ... }

    script 5 OPEN { ... }

 * Note that OPEN scripts do not take arguments.

### [4-2-6] Statements

    <statement>:
      <declaration-statement>
      <assignment-statement>
      <compound-statement>
      <switch-statement>
      <jump-statement>
      <selection-statement>
      <iteration-statement>
      <function-statement>
      <linespecial-statement>
      <print-statement>
      <control-statement>

#### [4-2-6-1] Declaration Statements

Delcaration statements create script variables.

    <declaration-statement>:
      int <variable> ;
      int <variable> , <variable> , ... ;

#### [4-2-6-1] Assignment Statements

Assigns an expression to a variable.

    <assignment-statement>:
      <variable> <assignment-operator> <expression> ;

    <assignment-operator>:
      =
      +=
      -=
      *=
      /=
      %=

* Note: An assignment of the form V <op>= E is equivalent to V = V <op> E.
  For example:

    A += 5;     is the same as
    A = A + 5;

#### [4-2-6-2] Compound Statements

    <compound-statement>:
      { <statement-list> }

    <statement-list>:
      <statement> <statement> <...>

#### [4-2-6-3] Switch Statements

A switch statement evaluates an integral expression and passes control to the
code following the matched case.

    <switch-statement>:
     switch ( <expression> ) { <labeled-statement-list> }

      <labeled-statement>:
        case <constant-expression> : <statement>
        default : <statement>

Example:

    switch (a)
    {
    case 1:         // when a == 1
        b = 1;      // .. this is executed,
        break;      // and this breaks out of the switch().

    case 2:         // when a == 2
        b = 8;      // .. this is executed,
                    // but there is no break, so it continues to the next
                    // case, even though a != 3.

    case 3:         // when a == 3
        b = 666;    // .. this is executed,
        break;      // and this breaks out of the switch().

    default:        // when none of the other cases match,
        b = 777;    // .. this is executed.
    }

 * Note for C users: While C only allows integral expressions in a switch
   statement, ACS allows full expressions such as "a + 10".

#### [4-2-6-4] Jump Statements

A jump statement passes control to another portion of the script.

    <jump-statement>:
      continue ;
      break ;
      restart ;

#### [4-2-6-5] Iteration Statements

    <iteration-statement>:
      while ( <expression> ) <statement>
      until ( <expression> ) <statement>
      do <statement> while ( <expression> ) ;
      do <statement> until ( <expression> ) ;
      for ( <assignment-statement> ; <expression> ; <assignment-statement> )
        <statement>

The continue, break and restart keywords can be used in an iteration
statement:

 - the continue keyword jumps to the end of the last <statement> in the
   iteration-statement.  The loop continues.

 - the break keyword jumps right out of the iteration-statement.

#### [4-2-6-6] Function Statements

A function statement calls a Hexen internal-function, or a Hexen
linespecial-function.

    <function-statement>:
      <internal-function> | <linespecial-statement>

    <internal-function>:
      <identifier> ( <expression> , ... ) ;
      <identifier> ( const : <constant-expression> , ... ) ;

    <linespecial-statement>:
      <linespecial> ( <expression> , ... ) ;
      <linespecial> ( const : <constant-expression> , ... ) ;

#### [4-2-6-7] Print Statements

    <print-statement>:
      print ( <print-type> : <expression> , ... ) ;
      printbold ( <print-type> : <expression> , ... ) ;

    <print-type>:
        s   string
        d   decimal
        c   constant

#### [4-2-6-8] Selection Statements

    <selection-statement>:
      if ( <expression> ) <statement>
      if ( <expression> ) <statement> else <statement>

#### [4-2-6-9] Control Statements

    <control-statement>:
      suspend ;         // suspends the script
      terminate ;       // terminates the script

### [4-2-7] Internal Functions

    void tagwait(int tag);
    ----------------------

     The current script is suspended until all sectors marked with
     <tag> are inactive.


    void polywait(int po);
    ----------------------

     The current script is suspended until the polyobj marked with
     <po> is incactive.


    void scriptwait(int script);
    ----------------------------

     The current script is suspended until the script specified by
     <script> has terminated.


    void delay(int ticks);
    ----------------------

     The current script is suspended for a time specified by <ticks>.
     A tick represents one cycle from a 35Hz timer.


    void changefloor(int tag, str flatname);
    ----------------------------------------

     The floor flat for all sectors marked with <tag> is changed to
     <flatname>.


    void changeceiling(int tag, str flatname);
    ------------------------------------------

     The ceiling flat for all sectors marked with <tag> is changed to
     <flatname>.


    int random(int low, int high);
    ------------------------------

     Returns a random number between <low> and <high>, inclusive.  The
     values for <low> and <high> range from 0 to 255.


    int lineside(void);
    -------------------

     Returns the side of the line the script was activated from.  Use
     the macros LINE_FRONT and LINE_BACK, defined in "defs.acs".


    void clearlinespecial(void);
    ----------------------------

     The special of the line that activated the script is cleared.


    int playercount(void);
    ----------------------

     Returns the number of active players.


    int gametype(void);
    -------------------

     Returns the type of game being played:
      GAME_SINGLE_PLAYER
      GAME_NET_COOPERATIVE
      GAME_NET_DEATHMATCH


    int gameskill(void);
    --------------------

     Returns the skill of the game being played:
      SKILL_VERY_EASY
      SKILL_EASY
      SKILL_NORMAL
      SKILL_HARD
      SKILL_VERY_HARD

     Example:
      int a;
      a = gameskill();

      switch( gameskill() )
      {
      case SKILL_VERY_EASY:
       ...
      case SKILL_VERY_HARD:
       ...
      }


    int timer(void);
    ----------------

     Returns the current leveltime in ticks.


    void sectorsound(str name, int volume);
    ---------------------------------------

     Plays a sound in the sector the line is facing.  <volume> has the
     range 0 to 127.


    void thingsound(int tid, str name, int volume);
    -----------------------------------------------

     Plays a sound at all things marked with <tid>.  <volume> has the
     range 0 to 127.


    void ambientsound(str name, int volume);
    ----------------------------------------

     Plays a sound that all players hear at the same volume.  <volume> has
     the range 0 to 127.


    void soundsequence(str name);
    -----------------------------

     Plays a sound sequence in the sector the line is facing.


    int thingcount(int type, int tid);
    ----------------------------------

     Returns a count of things in the world.  Use the thing type definitions
     in defs.acs for <type>.  Both <type> and <tid> can be 0 to force the
     counting to ignore that information.

     Examples:

     // Count all ettins that are marked with TID 28:
     c = thingcount(T_ETTIN, 28);

     // Count all ettins, no matter what their TID is:
     c = thingcount(T_ETTIN, 0);

     // Count all things with TID 28, no matter what their type is:
     c = thingcount(0, 28);


    void setlinetexture(int line, int side, int position, str texturename);
    -----------------------------------------------------------------------

     Sets a texture on all lines identified by <line>.  A line is identified by
     giving it the special Line_SetIdentification in a map editor.

     <side>:
      SIDE_FRONT
      SIDE_BACK
     <position>:
      TEXTURE_TOP
      TEXTURE_MIDDLE
      TEXTURE_BOTTOM

     Examples:

     setlinetexture(14, SIDE_FRONT, TEXTURE_MIDDLE, "ice01");
     setlinetexture(3, SIDE_BACK, TEXTURE_TOP, "forest03");


    void setlineblocking(int line, int blocking);
    ---------------------------------------------

     Sets the blocking (impassable) flag on all lines identified by <line>.
      <blocking>:
       ON
       OFF

     Example:

     setlineblocking(22, OFF);


    void setlinespecial(int line, int special, int arg1, int arg2,
                        int arg3, int arg4, int arg5);
    --------------------------------------------------------------

     Sets the line special and args on all lines identified by <line>.

# [5] Flats with special properties

    Lava        Lava does damage
    Water       Makes things sink
    Sludge      Makes things sink
    Ice         Changes friction

# [6] The MAPINFO lump

This is a lump in the .WAD that gives attributes to each map.  This entry
does not go with each map - there is only one MAPINFO lump in the entire
IWAD.  If you include a MAPINFO lump in a PWAD, make sure it's got
information for all the possible maps the player will be entering.

    map:        Number and name of map [1..60]
    warptrans:  Actual map number in case maps are not sequential [1..60]
    next:       Map to teleport to upon exit of timed deathmatch [1..60]
    cdtrack:    CD track to play during level
    cluster:    Defines what cluster level belongs to
    sky1:       Default sky texture; followed by speed
    sky2:       Alternate sky displayed in Sky2 sectors ; followed by speed
    doublesky:  parallax sky: sky2 behind sky1
    lightning:  Keyword indicating use of lightning on the level
                flashes from sky1 to sky2 (see also: IndoorLightning special)
    fadetable:  Lump Name of fade table {fogmap}

Example MapInfo entry:

    map 1 "Winnowing Hall"
    warptrans 1
    next 2
    cluster 1
    sky1 SKY2 2        ; 2 is the sky scroll speed
    sky2 SKY3 0        ; 0 means don't scroll sky
    lightning
    doublesky
    cdtrack 13

Note on "next" integer (for timed deathmatches):

In normal gameplay, there is no linear fashion in which the game
progresses from one level to another; you just go through a teleport
somewhere on a level, and it takes you to somewhere on another
level.  For -timer deathmatch, the game needs to know what level to
proceed to because it isn't always just the next higher level.

A note about the WARPTRANS keyword:  Maps are edited and named
MAPxx, where xx is a number from 01 to 63.  This is the number that
is used from within scripts when a map is referred to, and by the
MAP keyword in the MAPINFO lump.  However, the -warp option and the
warping cheat use a different set of numbers.  This different set of
numbers is set by the WARPTRANS keyword.  By default, the WARPTRANS
value is set to the same number as the map.  Our designers starting
making maps with numbers that had big gaps between them, and then
made the scripts refer to these numbers, so we needed a way to pack
all the map numbers into a continuous stream for the -warp option.
Also, the accepted range for a WARPTRANS value is 1-31.  Makes it
easy when using DM.

Note on "cluster" integer:

The game maps are divided into clusters.  When you enter a new cluster, you
can never again visit any of the levels from the previous cluster.  This
makes it so each individual save game only needs to backup map archives for
about 6-7 maps, and provides for a milestone marker of sorts for game play,
like an episode -- a Hexen backdrop and some text are given at the end of
each cluster.  If you don't enter a cluster, it defaults to 0.  The
commercial IWAD separates its 31 maps into 5 clusters.

# [7] PolyObjects

Polyobjs are one-sided lines that are built somewhere else on the map, and
then later translated to the desired start spot on the map at level load.

In building polyobjs, two different line specials can be used to determine
the line drawing order:

    Polyobj_ExplicitLine(polyNumber, orderNumber, polyMirror, sound);
    Polyobj_StartLine(polyNumber, polyMirror, sound);

Each polyobj should have a unique polyNumber, which is used in poly line
specials to refer to a particular polyobj.

polyMirror refers to a second polyobj that will "mirror" all actions of the
first polyobj.  For instance, if a polyobj is rotated to the right by 90
degrees, then that polyobj's mirror will rotate left 90 degrees.  Note that
having two polyobjs mirror each other is not considered to be a good thing,
but in general won't cause problems because a poly can only do one particular
action at a time.  Meaning:  if that poly that rotated left by 90 degrees
then mirrored the right-turning polyobj, the right-turning poly would ignore
any attempt to rotate it again, as it would already be being acted upon.

The last parameter to these specials refers to a particular sound type
that should play when the poly is moved/rotated.  See the section on
attaching sounds to a moving sector for more info.

**Polyobj_StartLine():**

A very basic special.  Place it on a particular polyobj line, and that line
will be the first line rendered on the polyobj.  The rendering order for all
other lines are determined by itterating through to the next line that has a
first point identical to the start line's second point.  The third line
rendered will be the next line that has a first point identical to the second
line's second point, and so on and so forth.  This method works well for
polyobjs that are convex, and has the advantage of leaving all but one line
free for other line specials.

**Polyobj_ExplicitLine:**

This special requires a bit more work to use.  Each line in the polyobj
defined using this special must use this line special.  Then, a value from
1-255 should be placed in orderNumber.  This defines the rendering order for
the lines, with a 1 being the first line rendered, and so on.  Useful for
non-convex polyobjs, but has the disadvantage of utilizing all line specials
on the poly.

## [7-1] Polyobj Start Spots and Anchor Points

Each polyobj must have an anchor point, and a startSpot.  The anchor is a
thing placed near the polyobj when it's created that defines the origin of
the polyobj, or the point in which it will rotate about.  The anchor (and all
polyobj lines) are directly translated to the polyobj startSpot.

Bottom line:  The anchor point is the point near the polyobj, and the
startSpot is the point on the actual map that defines the location of the
poly.

There are two different types of startSpots:  crushing and non-crushing.
Pretty obvious what the difference is:  if the poly strikes an object, it'll
first attempt to move it.  If that fails it will either try to damage the
object, or just stop moving depending upon the type of startSpot.

Please note that the ANGLE field of the startSpot and anchor points should be
equal to the polyNumber that was previously defined for that particular
polyobj.  The polyobj stuff was done before any of the TID/thing special code
was implimented, so Raven did this temporary hack, which turned permanent, as
the designers had already done a ton of polyobjs, and didn't want to have to
go back and replace them.

# APPENDIX [A]: List of Spawnable Objects

Use these identifiers for the Thing_Spawn() and Thing_SpawnNoFog()
specials:

    T_NONE
    T_CENTAUR
    T_CENTAURLEADER
    T_DEMON
    T_ETTIN
    T_FIREGARGOYLE
    T_WATERLURKER
    T_WATERLURKERLEADER
    T_WRAITH
    T_WRAITHBURIED
    T_FIREBALL1
    T_MANA1
    T_MANA2
    T_ITEMBOOTS
    T_ITEMEGG
    T_ITEMFLIGHT
    T_ITEMSUMMON
    T_ITEMTPORTOTHER
    T_ITEMTELEPORT
    T_BISHOP
    T_ICEGOLEM
    T_BRIDGE
    T_DRAGONSKINBRACERS
    T_ITEMHEALTHPOTION
    T_ITEMHEALTHFLASK
    T_ITEMHEALTHFULL
    T_ITEMBOOSTMANA
    T_FIGHTERAXE
    T_FIGHTERHAMMER
    T_FIGHTERSWORD1
    T_FIGHTERSWORD2
    T_FIGHTERSWORD3
    T_CLERICSTAFF
    T_CLERICHOLY1
    T_CLERICHOLY2
    T_CLERICHOLY3
    T_MAGESHARDS
    T_MAGESTAFF1
    T_MAGESTAFF2
    T_MAGESTAFF3
    T_MORPHBLAST
    T_ROCK1
    T_ROCK2
    T_ROCK3
    T_DIRT1
    T_DIRT2
    T_DIRT3
    T_DIRT4
    T_DIRT5
    T_DIRT6
    T_ARROW
    T_DART
    T_POISONDART
    T_RIPPERBALL
    T_STAINEDGLASS1
    T_STAINEDGLASS2
    T_STAINEDGLASS3
    T_STAINEDGLASS4
    T_STAINEDGLASS5
    T_STAINEDGLASS6
    T_STAINEDGLASS7
    T_STAINEDGLASS8
    T_STAINEDGLASS9
    T_STAINEDGLASS0
    T_BLADE
    T_ICESHARD
    T_FLAME_SMALL
    T_FLAME_LARGE
    T_MESHARMOR
    T_FALCONSHIELD
    T_PLATINUMHELM
    T_AMULETOFWARDING
    T_ITEMFLECHETTE
    T_ITEMTORCH
    T_ITEMREPULSION
    T_MANA3
    T_PUZZSKULL
    T_PUZZGEMBIG
    T_PUZZGEMRED
    T_PUZZGEMGREEN1
    T_PUZZGEMGREEN2
    T_PUZZGEMBLUE1
    T_PUZZGEMBLUE2
    T_PUZZBOOK1
    T_PUZZBOOK2
    T_METALKEY
    T_SMALLMETALKEY
    T_AXEKEY
    T_FIREKEY
    T_GREENKEY
    T_MACEKEY
    T_SILVERKEY
    T_RUSTYKEY
    T_HORNKEY
    T_SERPENTKEY
    T_WATERDRIP
    T_TEMPSMALLFLAME
    T_PERMSMALLFLAME
    T_TEMPLARGEFLAME
    T_PERMLARGEFLAME
    T_DEMON_MASH
    T_DEMON2_MASH
    T_ETTIN_MASH
    T_CENTAUR_MASH
    T_THRUSTSPIKEUP
    T_THRUSTSPIKEDOWN
    T_FLESH_DRIP1
    T_FLESH_DRIP2
    T_SPARK_DRIP

# APPENDIX [B]: List of Activateable/Deactivateable Objects

Activatable:

    MT_ZTWINEDTORCH             Lights torch
    MT_ZTWINEDTORCH_UNLIT       Lights torch
    MT_ZWALLTORCH               Lights torch
    MT_ZWALLTORCH_UNLIT         Lights torch
    MT_ZGEMPEDESTAL             Makes gem appear
    MT_ZWINGEDSTATUENOSKULL     Makes skull appear in hands
    MT_THRUSTFLOOR_UP           Raises thrust spike (if lowered)
    MT_THRUSTFLOOR_DOWN         Raises thrust spike
    MT_ZFIREBULL                Lights flames
    MT_ZFIREBULL_UNLIT          Lights flames
    MT_ZBELL                    Rings bell
    MT_ZCAULDRON                Lights flames
    MT_ZCAULDRON_UNLIT          Lights flames
    MT_FLAME_SMALL              Ignites flame
    MT_FLAME_LARGE              Ignites flame
    MT_BAT_SPAWNER              Start bat spawning

Deactivatable:

    MT_ZTWINEDTORCH             Extinguish torch
    MT_ZTWINEDTORCH_UNLIT       Extinguish torch
    MT_ZWALLTORCH               Extinguish torch
    MT_ZWALLTORCH_UNLIT         Extinguish torch
    MT_THRUSTFLOOR_UP           Lower thrust spike
    MT_THRUSTFLOOR_DOWN         Lower thrust spike
    MT_ZFIREBULL                Extinguish torch
    MT_ZFIREBULL_UNLIT          Extinguish torch
    MT_ZCAULDRON                Extinguish torch
    MT_ZCAULDRON_UNLIT          Extinguish torch
    MT_FLAME_SMALL              Extinguish torch
    MT_FLAME_LARGE              Extinguish torch
    MT_BAT_SPAWNER              Stop bat spawning

# APPENDIX [C]: List of THINGS that require arguments

These THINGS ignore their special types, and use the arg0..arg5 fields
for their own purposes:

    Type: 10225        Bat Spawner
        arg0:   frequency of spawn (1=fastest, 10=slowest)
        arg1:   spread angle (0..255)
        arg2:   unused
        arg3:   duration of bats (in octics)
        arg4:   turn amount per move (in degrees [0..255])

    Type: 10000        Fog Spawner
        arg0:    movement speed [0..10] (10 == fastest)
        arg1:    spread angle [0..128] (128 == 180 degrees)
        arg2:    Frequency of spawn [1..10] (1 == fastest)
        arg3:    Fog Lifetime [0..255] (5 == 1 second)
        arg4:    unused

    Type: 10001        Fog Patch Small
        arg0:    movement speed [0..10] (10 == fastest)
        arg1:    unused
        arg2:    unused
        arg3:    Fog Lifetime [0..255] (5 == 1 second)
        arg4:    Boolean: (0 == not moving)

    Type: 10002        Fog Patch Medium
        arg0:    movement speed [0..10] (10 == fastest)
        arg1:    unused
        arg2:    unused
        arg3:    Fog Lifetime [0..255] (5 == 1 second)
        arg4:    Boolean: (0 == not moving)

    Type: 10003        Fog Patch Large
        arg0:    movement speed [0..10] (10 == fastest)
        arg1:    unused
        arg2:    unused
        arg3:    Fog Lifetime [0..255] (5 == 1 second)
        arg4:    Boolean: (0 == not moving)

    Type: 254          Dragon Lich
        arg0:    TID of possible destination (required)
        arg1:    TID of possible destination (optional)
        arg2:    TID of possible destination (optional)
        arg3:    TID of possible destination (optional)
        arg4:    TID of possible destination (optional)

The dragon lich also requires mapspots placed around the map with
its args containing TIDs of possible destinations, making up to 5
destinations possible from each position.  The choice of next
destination is random.  Note that the dragon lich's first
destination is the first thing that it can locate that has a TID
identical to it's own.

    Type: 10200        Korax
        TIDs:
            245     Korax's mapthing
            249     Teleport destination (MapSpots)
        Scripts:
            249     Run when korax health falls below half
            250-254 Randomly run by korax as commands
            255     Run upon death of korax

# APPENDIX [D]: Sector Specials

The following numbers are used in the sector.type field [see DOOM
specs.]:

    1   Light_Phased
    2   LightSequenceStart
    3   LightSequenceSpecial1
    4   LightSequenceSpecial2

These specials deal with phased lightning ("moving lights").  Two
different ways to go about doing phased lighting:  automatic, or
by-hand.  The automatic method is (obviously) more convenient, but
the by-hand method is more flexible.  Light_Phased is the by-hand
special.  Place it on a sector, then set the sector's lightlevel to a
phase index (0-63).  As you place the special on nearby sectors,
increment the index for each sector.

Or, to use the LightSequence specials, just place the LightSequence
special on a sector.  Then, for each additional sector, alternate
between LightSequenceSpecial1 & LightSequenceSpecial2.

For instance, if you wanted phased lightning to flow up a staircase,
you could either place Light_Phased on each step, and change the
phase index (lightlevel) accordingly.  Or, you could place
LightSequenceStart on the bottom step (and set that step's lightlevel
to something mid-ranged:  80-128 are pretty nice values), and then
let the game calculate the phase indices for each step by placing the
LightSequenceSpecial specials on all other steps.  Note that for the
LightSequenceSpecial specials to have proper lighting, set their
lightlevels to zero, which causes it to use the previous sector's
lightlevel.  Hence, that "nice value" which was placed on the first
step will iterate through all the other steps.  If a step's
lightlevel is not zero, then that value will filter down to all other
steps after it.

    26    Stairs_Special1
    27    Stairs_Special2

Used by action specials that build stairs.

    199    Light_IndoorLightning1

Dimmer effect during lightning flash.  Used for indoor areas, which
are normally not affected by lightning.

    198 Light_IndoorLightning2

Same as 1, but brighter.

    200    Sky2

Use the alternate sky specified in the mapinfo lump.

    201    Scroll_North_Slow
    202    Scroll_North_Medium
    203    Scroll_North_Fast
    204    Scroll_East_Slow
    205    Scroll_East_Medium
    206    Scroll_East_Fast
    207    Scroll_South_Slow
    208    Scroll_South_Medium
    209    Scroll_South_Fast
    210    Scroll_West_Slow
    211    Scroll_West_Medium
    212    Scroll_West_Fast
    213    Scroll_NorthWest_Slow
    214    Scroll_NorthWest_Medium
    215    Scroll_NorthWest_Fast
    216    Scroll_NorthEast_Slow
    217    Scroll_NorthEast_Medium
    218    Scroll_NorthEast_Fast
    219    Scroll_SouthEast_Slow
    220    Scroll_SouthEast_Medium
    221    Scroll_SouthEast_Fast
    222    Scroll_SouthWest_Slow
    223    Scroll_SouthWest_Medium
    224    Scroll_SouthWest_Fast

These all scroll floor flats in their respective directions.  They
also move any objects in that direction.

# APPENDIX [E]: Action Specials

These are the specials found in the THING.special and LINEDEF.special
fields.

## [E-1] Floor and Ceiling Specials

    20:Floor_LowerByValue / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]
        height:     relative height of move in pixels

Moves the floor of all sectors identified by 'tag'.

    21:Floor_LowerToLowest / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]

Lowers floor to lowest adjacent sectors' floor.

    22:Floor_LowerToNearest / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]

Lowers floor to next lower adjacent sector's floor.

    23:Floor_RaiseByValue / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]
        height:     relative height of move in pixels

Moves the floor of all sectors identified by 'tag'.

    24:Floor_RaiseToHighest / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]

Raises floor to highest adjacent sectors' floor.

    25:Floor_RaiseToNearest / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]

Raises floor to next higher adjacent sector's floor.

    28:Floor_RaiseAndCrush / tag / speed / crush / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]
        crush:      damage done by crush

Raises floor to ceiling and does crushing damage.

    35:Floor_RaiseByValueTimes8 / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]
        height:     relative height of move in 8 pixel units

Raises the floor in increments of 8 units.

    36:Floor_LowerByValueTimes8 / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move [0..255]
        height:     relative height of move in 8 pixel units

Lowers the floor in increments of 8 units.

    46:Floor_CrushStop / tag / arg2 / arg3 / arg4 / arg5
        tag:        tag of affected sector

Turns off a crushing floor.

    66:Floor_LowerInstant / tag / arg2 / height / arg4 / arg5
        tag:        tag of affected sector
        height:     relative height in units of 8 pixels

Moves the floor down instantly by a specified amount.

    67:Floor_RaiseInstant / tag / arg2 / height / arg4 / arg5
        tag:        tag of affected sector
        height:     relative height in units of 8 pixels

Moves the floor up instantly by a specified amount.

    68:Floor_MoveToValueTimes8 / tag / speed / height / negative / arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     absolute value in 8 pixel units of destination height
        negative:   boolean (true if height is negative)

Move floor to an absolute height.

    40:Ceiling_LowerByValue / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     relative height of move in pixels

Relative ceiling move.

    41:Ceiling_RaiseByValue / tag / speed / height / arg4 /arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     relative height of move in pixels

Relative ceiling move.

    42:Ceiling_CrushAndRaise / tag / speed / crush / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        crush:      damage of crush

Lowers ceiling to crush and raises (continual until stopped)

    43:Ceiling_LowerAndCrush / tag / speed / crush / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        crush:      damage of crush

Lowers ceiling to floor and stops.

    44:Ceiling_CrushStop / tag / arg2 / arg3 / arg4 / arg5
        tag:        tag of affected sector

Stop a crushing ceiling.

    45:Ceiling_CrushRaiseAndStay / tag / speed / crush / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        crush:      damage of crush

Lowers ceiling to crush, raises and stays.

    69:Ceiling_MoveToValueTimes8 / tag / speed / height / negative / arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     absolute value in 8 pixel units of destination height
        negative:   boolean (true if height is negative)

Moves ceiling to absolute height.

    95:FloorAndCeiling_LowerByValue / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     absolute value in 8 pixel units of destination height

Relative move of both floor and ceiling.

    96:FloorAndCeiling_RaiseByValue / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        height:     absolute value in 8 pixel units of destination height

Relative move of both floor and ceiling.

    60:Plat_PerpetualRaise / tag / speed / delay / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        delay:      delay before reversing direction

Continually raises and lowers platform.

    61:Plat_Stop / tag / arg2 / arg3 / arg4 / arg5
        tag:        tag of affected sector

Stops a PerpectualRaise platform.

    62:Plat_DownWaitUpStay / tag / speed / delay / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        delay:      delay before reversing direction

One cycle of lowering and raising.

    63:Plat_DownByValue / tag / speed / delay / height / arg5
        tag:        tag of affected sector
        speed:      speed of move
        delay:      delay before reversing direction
        height:     relative height in 8 pixel units

Relative platform move.

    64:Plat_UpWaitDownStay / tag / speed / delay / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of move
        delay:      delay before reversing direction

One cycle of raising and lowering.

    65:Plat_UpByValue / tag / speed / delay / height / arg5
        tag:        tag of affected sector
        speed:      speed of move
        delay:      delay before reversing direction
        height:     relative height

Relative platform move.

    29:Pillar_Build / tag / speed / height / arg4 / arg5
        tag:        tag of affected sector
        speed:      speed of build
        height:     height (relative to floor) where

Makes the floor meet the ceiling.

    30:Pillar_Open / tag / speed / f_height / c_height / arg5
        tag:        tag of affected sector
        speed:      speed of build
        f_height:   relative height to move floor down
        c_height:   relative height to move ceiling up

Makes the floor and the ceiling meet by moving both.

    94:Pillar_BuildAndCrush / tag / speed / height / crush / arg5
        tag:        tag of affected sector
        speed:      speed of build
        height:     height (relative to floor) where floor meets ceiling
        crush:      damage from crushing

## [E-2] Stair Specials

These stair building specials find the sector with 'tag' and
build stairs by traversing adjacent sector marked with the
StairSpecial1 and StairSpecial2.  These specials must alternate
between the two and must not branch.

    26:Stairs_BuildDown / tag / speed / height / delay / reset
        tag:        tag of sector to start in
        speed:      speed of build [0.255]
        height:     height of step in pixels
        delay:      delay between steps in tics
        reset:      delay before stairs to reset (0==no reset)

    27:Stairs_BuildUp / tag / speed / height / delay / reset
        tag:        tag of sector to start in
        speed:      speed of build [0.255]
        height:     height of step in pixels
        delay:      delay between steps in tics
        reset:      delay before stairs to reset (0==no reset)

    31:Stairs_BuildDownSync / tag / speed / height / reset / arg5
        tag:        tag of sector to start in
        speed:      speed of build [0.255]
        height:     height of step in pixels
        reset:      delay before stairs to reset (0==no reset)

    32:Stairs_BuildUpSync / tag / speed / height / reset / arg5
        tag:        tag of sector to start in
        speed:      speed of build [0.255]
        height:     height of step in pixels
        reset:      delay before stairs to reset (0==no reset)

## [E-3] Door Specials

    10:Door_Close / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector or zero if line is part of door
        speed:      speed of move

Closes a door sector.

    11:Door_Open / tag / speed / arg3 / arg4 / arg5
        tag:        tag of affected sector or zero if line is part of door
        speed:      speed of move

Opens a door sector.

    12:Door_Raise / tag / speed / delay / arg4 / arg5
        tag:        tag of affected sector or zero if line is part of door
        speed:      speed of move
        delay:      delay before door lowers

    13:Door_LockedRaise / tag / speed / delay / lock / arg5
        tag:        tag of affected sector or zero if line is part of door
        speed:      speed of move
        delay:      delay before door lowers
        lock:       key number that will unlock door (see key numbers)

Raises a door if correct key is in inventory of triggering player.

## [E-4] Script Specials

    80:ACS_Execute / script / map / s_arg1 / s_arg2 / s_arg3
        script:     script number to execute
        map:        map which contains the script

    81:ACS_Suspend / script / map / arg3 / arg4 / arg5
        script:     script number to suspend
        map:        map which contains the script

    82:ACS_Terminate / script / map / arg3 / arg4 / arg5
        script:     script number to suspend
        map:        map which contains the script

    83:ACS_LockedExecute / script / map / s_arg1 / s_arg2 / lock
        script:     script number to suspend
        map:        map which contains the script
        lock:       key number needed to run script (see key numbers)

## [E-5] Light Specials

    110:Light_RaiseByValue / tag / value / arg3 / arg4 / arg5
        tag:        tag of affected sector
        value:      relative value of light level change

    111:Light_LowerByValue / tag / value / arg3 / arg4 / arg5
        tag:        tag of affected sector
        value:      relative value of light level change

    112:Light_ChangeToValue / tag / value / arg3 / arg4 / arg5
        tag:        tag of affected sector
        value:      absolute value of light level change

    113:Light_Fade / tag / value / tics / arg4 / arg5
        tag:        tag of affected sector
        value:      absolute value of light level change
        tics:       number of tics to fade to light level

    114:Light_Glow / tag / upper / lower / tics / arg5
        tag:        tag of affected sector
        upper:      brightest light level
        lower:      lowest light level
        tics:       number of tics between light changes

    115:Light_Flicker / tag / upper / lower / arg4 / arg5
        tag:        tag of affected sector
        upper:      brightest light level
        lower:      lowest light level

    116:Light_Strobe / tag / upper / lower / u-tics / l-tics
        tag:        tag of affected sector
        upper:      brightest light level
        lower:      lowest light level
        u-tics:     tics to stay at upper light level
        l-tics:     tics to stay at lower light level

## [E-6] Miscellaneous Specials

    121:Line_SetIdentification / line / arg2 / arg3 / arg4 / arg5
        line:       unique id of this line

The script functions setlineblocking, setlinespecial, and
setlinetexture use the ID specified here to identify lines.

    100:Scroll_Texture_Left / speed / arg2 / arg3 / arg4 / arg5
    101:Scroll_Texture_Right / speed / arg2 / arg3 / arg4 / arg5
    102:Scroll_Texture_Up / speed / arg2 / arg3 / arg4 / arg5
    103:Scroll_Texture_Down / speed / arg2 / arg3 / arg4 / arg5
        speed:      speed of scroll in pixels

    129:UsePuzzleItem / item / script / s_arg1 / s_arg2 / s_arg3
        item:       item number needed to activate
        script:     script to run upon activation

Runs a script upon use of appropriate puzzle item:

    0   ZZ_Skull
    1   ZZ_BigGem
    2   ZZ_GemRed
    3   ZZ_GemGreen1
    4   ZZ_GemGreen2
    5   ZZ_GemBlue1
    6   ZZ_GemBlue2
    7   ZZ_Book1
    8   ZZ_Book2
    9   ZZ_Skull2
    10  ZZ_FWeapon
    11  ZZ_CWeapon
    12  ZZ_MWeapon
    13  ZZ_Gear
    14  ZZ_Gear2
    15  ZZ_Gear3
    16  ZZ_Gear4

    140:Sector_ChangeSound / tag / sound / arg3 / arg4 /  arg5
        tag:        tag of sector to contain sound
        sound:      sound to be played - see sector sounds

    120:Earthquake / intensity / duration / damrad / tremrad / tid
        intensity:  strength of earthquake in richters [1..9]
        duration:   duration in tics [1..255]
        damrad:     radius of damage in 64x64 cells [0..255]
        tremrad:    radius of tremor in 64x64 cells [0..255]
        tid:        TID of map thing(s) for quake foci

Creates an earthquake at all matching foci.

    74:Teleport_NewMap / map / position / arg3 / arg4 / arg5
        map:        map to teleport to
        position:   corresponds to destination player start spot arg0.

Teleports the player to a new map and to the player start spot
whose arg0 member matches 'position.'

    75:Teleport_EndGame / arg1 / arg2 / arg3 / arg4 / arg5

Ends game and runs finale script.
In deathmatch, teleports to level 1.

    70:Teleport / tid / arg2 / arg3 / arg4 / arg5
        tid:        TID of destination

Teleports triggering object to MapSpot with tid.

    71:Teleport_NoFog / tid / arg2 / arg3 / arg4 / arg5

Same as teleport, but silent with no fog sprite.

## [E-7] Thing Specials

    72:ThrustThing / angle / distance / arg3 / arg4 / arg5
        angle:      byte angle to thrust [0..255]
        distance:   distance to thrust

    73:DamageThing / damage / arg2 / arg3 / arg4 / arg5
        damage:     amount of damage

    130:Thing_Activate / tid / arg2 / arg3 / arg4 / arg5
        tid:        TID of thing to activate (see activatable things)

    131:Thing_Deactivate / tid / arg2 / arg3 / arg4 / arg5
        tid:        TID of thing to deactivate (see deactivatable things)

    132:Thing_Remove / tid / arg2 / arg3 / arg4 / arg5
        tid:        TID of thing to remove

    133:Thing_Destroy / tid / arg2 / arg3 / arg4 / arg5
        tid:        TID of affected thing

Puts thing into its death state.

    134:Thing_Projectile / tid / type / angle / speed / vspeed
        tid:        TID of spawn location
        type:       Type of thing to spawn (see spawnable things)
        angle:      byte angle of projectile
        speed:      speed of projectile
        vspeed:     vertical speed

Spawns a projectile.

    136:Thing_ProjectileGravity / tid / type / angle / speed / vspeed
        tid:        TID of spawn location
        type:       Type of thing to spawn (see spawnable things)
        angle:      byte angle of projectile
        speed:      speed of projectile
        vspeed:     vertical speed

Spawns a projectile with gravity.

    135:Thing_Spawn / tid / type / angle / arg4 / arg5
        tid:        TID of spawn location
        type:       Type of thing to spawn (see spawnable things)
        angle:      byte angle of thing to face

Spawns a thing.

    137:Thing_SpawnNoFog / tid / type / angle / arg4 / arg5
        tid:        TID of spawn location
        type:       Type of thing to spawn (see spawnable things)
        angle:      byte angle of projectile

Spawns a thing silently.

## [E-8] PolyObject Specials

    1:Polyobj_StartLine / po / mirror / sound / arg4 / arg5
        po:         refer to a particular polyobj
        mirror:     poly that will mirror the moves of this poly
        sound:      See Section 5:  Sector Sound

    2:Polyobj_RotateLeft / po / speed / angle / arg4 / arg5
        po:         polyobj
        speed:      speed
        angle:      byte angle to rotate

    3:Polyobj_RotateRight / po / speed / angle / arg4 / arg5
        po:         polyobj
        speed:      speed
        angle:      byte angle to rotate

    4:Polyobj_Move / po / speed / angle / distance / arg5
        po:         polyobj
        speed:      speed
        angle:      byte angle to move along
        distance:   byte distance to move

    5:Polyobj_ExplicitLine / po / order / mirror / sound / arg5
        po:         polyobj
        order:      rendering order of this line
        mirror:     poly that will mirror the moves of this poly
        sound:      See Section 5:  Sector Sound

    6:Polyobj_MoveTimes8 / po / speed / angle / distance / arg5
        po:         polyobj
        speed:      speed
        angle:      byte angle
        distance:   byte distance to move in units of 8

    7:Polyobj_DoorSwing / po / speed / angle / delay / arg5
        po:         polyobj
        speed:      speed
        angle:      byte angle
        delay:      delay in tics

    8:Polyobj_DoorSlide / po / speed / angle / distance / delay
        po:         polyobj
        speed:      speed
        angle:      byte angle
        distance:   byte distance
        delay:      delay in tics

    90:Polyobj_OR_RotateLeft / po / speed / angle / arg4 / arg5
    91:Polyobj_OR_RotateRight / po / speed / angle / arg4 / arg5
    92:Polyobj_OR_Move / po / speed / angle / distance / arg5
    93:Polyobj_OR_MoveTimes8 / po / speed / angle / distance / arg5

The OR stands for OverRide.  As stated before, each poly can only be doing a
single action at a time.  This poses a problem with perpetual polyobjs, since
they are already moving, the designer cannot do anything else with them.
However, using these functions the designer can override the code to not
allow a poly to concurrently execute more than one action, and force a poly
to do the other action as well.

# APPENDIX [F]: Sector Sounds for ChangeSectorSound() special

    1   heavy
    2   metal
    3   creak
    4   silence
    5   lava
    6   water
    7   ice
    8   earth
    9   metal2

# APPENDIX [G]: Key Numbers

These are referenced by the DoorRaiseLocked() and ACS_ExecuteLocked()
specials.

    1   steel key
    2   cave key
    3   axe key
    4   fire key
    5   emerald key
    6   dungeon key
    7   silver key
    8   rusted key
    9   horn key
    10  swamp key
    11  castle key


