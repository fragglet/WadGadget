# ACS Assembler format

WadGadget has a built-in disassembler / assembler for BEHAVIOR lumps, which
contain the compiled bytecode for the Action Code Script (ACS) scripting
system that was introduced in Hexen. This page documents the assembly format
that is supported by WadGadget.

Note that the disassembly is at a lower level than the ACS language that is
usually used for writing these scripts. You may find it more convenient to use
a tool like DEACC which decompiles to the original ACS language.

At present, only the original ACS0 instruction set (as supported by Hexen) is
supported by WadGadget. Some source ports such as ZDoom and Eternity Engine
support an extended instruction set; support for this may be added in the
future.

## Brief description of the ACS VM

The ACS VM is a stack machine and most instructions perform some modification
to the stack, by "popping" arguments from the stack and "pushing" a result
back to it. For example, the following will add two to the value at the top of
the stack:

  PushNumber 2
  Add

Some instructions have "direct" equivalents that take direct arguments from
bytecode instead of popping them from the stack. For example,

  DelayDirect 5

is equivalent to

  PushNumber 5
  Delay

A full description of the VM is beyond the scope of this document.

## Comment syntax

Comments start with a ';' character and continue until the end of line. For
example:

  ; This is a comment

## Script statements

Linedefs in the level can trigger scripts to start, and the starting point of
a script is defined using a Script statement. For example:

  Script 1

Scripts can take up to three parameters, which are stored in the linedef that
triggers them. If a script takes parameters then the statement looks like:

  Script 1 (2)

for a statement with two parameters.

## String statements

ACS scripts sometimes use character strings. For example, the
ChangeFloorDirect instruction can be used to change the floor texture of
sectors with a particular tag. In this case, you define a string containing
the texture name and pass that string's ID to ChangeFloorDirect. For example:

  String 2 = "X_001"
  ; Change all sectors with tag 60 to have floor texture X_001:
  ChangeFloorDirect 60 2

## Labels

Some instructions jump to other locations in the code. For example, the
following repeatedly decrements the value of script variable 0 until it is
equal to zero:

  loop:
    DecScriptVar   0
    PushScriptVar  0
    IfGoto         loop

Instructions that take a jump location are: Goto; IfGoto; IfNotGoto; and
CaseGoto.

## Instructions

The following is a list of instructions recognized by the assembler, along
with a brief description of each in C-like syntax.

  Instruction         Args  Description / Pseudocode
  NOP                 0     Does nothing
  Terminate           0     Terminate script
  Suspend             0     Suspend script
  PushNumber          1     Push(arg0)
  LSpec1              1     LinedefSpecial(arg0, Pop())
  LSpec2              1     a2=Pop(); LinedefSpecial(arg0, Pop(), a2)
  LSpec3              1     a2=Pop(); a1=Pop();
                            LinedefSpecial(arg0, Pop(), a1, a2)
  LSpec4              1     a3=Pop(); a2=Pop(); a1=Pop();
                            LinedefSpecial(arg0, Pop(), a1, a2, a3)
  LSpec5              1     a4=Pop(); a3=Pop(); a2=Pop(); a1=Pop();
                            LinedefSpecial(arg0, Pop(), a1, a2, a3, a4)
  LSpec1Direct        2     LinedefSpecial(arg0, arg1)
  LSpec2Direct        3     LinedefSpecial(arg0, arg1, arg2)
  LSpec3Direct        4     LinedefSpecial(arg0, arg1, arg2, arg3)
  LSpec4Direct        5     LinedefSpecial(arg0, arg1, arg2, arg3, arg4)
  LSpec5Direct        6     LinedefSpecial(arg0, arg1, arg2, arg3, arg4, arg5)
  Add                 0     Push(Pop() + Pop())
  Subtract            0     tmp=Pop(); Push(Pop() - tmp)
  Multiply            0     Push(Pop() * Pop())
  Divide              0     tmp=Pop(); Push(Pop() / tmp)
  Modulus             0     tmp=Pop(); Push(Pop() % tmp)
  EQ                  0     Push(Pop() == Pop())
  NE                  0     Push(Pop() != Pop())
  LT                  0     tmp=Pop(); Push(Pop() < tmp)
  GT                  0     tmp=Pop(); Push(Pop() > tmp)
  LE                  0     tmp=Pop(); Push(Pop() <= tmp)
  GE                  0     tmp=Pop(); Push(Pop() >= tmp)
  AssignScriptVar     1     ScriptVars[arg0] = Pop()
  AssignMapVar        1     MapVars[arg0] = Pop()
  AssignWorldVar      1     WorldVars[arg0] = Pop()
  PushScriptVar       1     Push(ScriptVars[arg0])
  PushMapVar          1     Push(MapVars[arg0])
  PushWorldVar        1     Push(WorldVars[arg0])
  AddScriptVar        1     ScriptVars[arg0] += Pop()
  AddMapVar           1     MapVars[arg0] += Pop()
  AddWorldVar         1     WorldVars[arg0] += Pop()
  SubScriptVar        1     ScriptVars[arg0] -= Pop()
  SubMapVar           1     MapVars[arg0] -= Pop()
  SubWorldVar         1     WorldVars[arg0] -= Pop()
  MulScriptVar        1     ScriptVars[arg0] *= Pop()
  MulMapVar           1     MapVars[arg0] *= Pop()
  MulWorldVar         1     WorldVars[arg0] *= Pop()
  DivScriptVar        1     ScriptVars[arg0] /= Pop()
  DivMapVar           1     MapVars[arg0] /= Pop()
  DivWorldVar         1     WorldVars[arg0] /= Pop()
  ModScriptVar        1     ScriptVars[arg0] %= Pop()
  ModMapVar           1     MapVars[arg0] %= Pop()
  ModWorldVar         1     WorldVars[arg0] %= Pop()
  IncScriptVar        1     ScriptVars[arg0]++
  IncMapVar           1     MapVars[arg0]++
  IncWorldVar         1     WorldVars[arg0]++
  DecScriptVar        1     ScriptVars[arg0]--
  DecMapVar           1     MapVars[arg0]--
  DecWorldVar         1     WorldVars[arg0]--
  Goto                1     Unconditional jump to arg0
  IfGoto              1     Conditional jump to arg0 if Pop() != 0
  Drop                0     Discard top of stack
  Delay               0     Suspend script for Pop() tics
  DelayDirect         1     Suspend script for arg0 tics
  Random              0     max=Pop(); min=Pop(); Push(value in range min...max)
  RandomDirect        2     Push(random value in range arg0...arg1)
  ThingCount          0     type=Pop(); tid=Pop(); Push(count of things)
  ThingCountDirect    2     type=arg0; tid=arg1; Push(count of things)
  TagWait             0     Sleep until linedef with tag Pop() is complete
  TagWaitDirect       1     Sleep until linedef with tag arg0 is complete
  PolyWait            0     Sleep until polyobject with tag Pop() is complete
  PolyWaitDirect      1     Sleep until polyobject with tag arg0 is complete
  ChangeFloor         0     texture=Strings[Pop()]; tag=Pop();
                            change floor texture for sectors with tag
  ChangeFloorDirect   2     texture=Strings[arg1]; tag=arg0;
                            change floor texture with sectors with tag
  ChangeCeiling       0     Same as ChangeFloor but for ceiling textures
  ChangeCeilingDirect 2     Same as ChangeFloorDirect but for ceiling textures
  Restart             0     Restart current script
  AndLogical          0     Push(Pop() && Pop())
  OrLogical           0     Push(Pop() || Pop())
  AndBitwise          0     Push(Pop() & Pop())
  OrBitwise           0     Push(Pop() | Pop())
  EorBitwise          0     Push(Pop() ^ Pop())
  NegateLogical       0     Push(!Pop())
  LShift              0     tmp=Pop(); Push(Pop() << tmp)
  RShift              0     tmp=Pop(); Push(Pop() >> tmp)
  UnaryMinus          0     Push(-Pop())
  IfNotGoto           1     Conditional jump to arg0 if Pop() == 0
  LineSide            0     Push(side of line that triggered script activation)
  ScriptWait          0     id=Pop(); Sleep until script #id finishes
  ScriptWaitDirect    1     Sleep until script #arg0 finishes
  ClearLineSpecial    0     Set special type of script's activating line to zero
  CaseGoto            2     Peek top of stack; if == arg0, pop and jump to arg1
  BeginPrint          0     PrintBuffer = ""
  EndPrint            0     Display PrintBuffer to player who started script
  PrintString         0     PrintBuffer += Strings[Pop()]
  PrintNumber         0     PrintBuffer += IntToString(Pop())
  PrintCharacter      0     PrintBuffer += Char(Pop())
  PlayerCount         0     Push(count of players in game)
  GameType            0     Push(type of game; 0=sp, 1=coop, 2=deathmatch)
  GameSkill           0     Push(game skill level; range 0-4)
  Timer               0     Push(level time in tics)
  SectorSound         0     Same as AmbientSound, but in activating line's sector
  AmbientSound        0     Start at volume Pop(), sound effect Strings[Pop()]
  SoundSequence       0
  SetLineTexture      0     Change line texture, texture=Strings[Pop()];
                            position=Pop(); side=Pop(); tag=Pop()
  SetLineBlocking     0     Change line blocking flag; blocking=Pop(); tag=Pop()
  SetLineSpecial      0     Change line special type; arg5=Pop(); arg4=Pop();
                            arg3=Pop(); arg2=Pop(); arg1=Pop()
  ThingSound          0     Start sound effect with thing as origin;
                            volume=Pop(); sound=Strings[Pop()]; thing_id=Pop()
  EndPrintBold        0     Same as EndPrint, text shown in yellow to all players
