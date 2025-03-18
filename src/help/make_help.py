#!/usr/bin/env python3
#
# Copyright(C) 2024 Simon Howard
#
# You can redistribute and/or modify this program under the terms of
# the GNU General Public License version 2 as published by the Free
# Software Foundation, or any later version. This program is
# distributed WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

import os
import re
import sys
import array

# Print the header files.
print("// This file is auto-generated.")
print("#include <stdlib.h>")
print('#include "help_text.h"')

# Print all md files as char arrays in hex format.
for filename in sys.argv[1:]:
      
        bytes = array.array('B', open(filename, "rb").read())
        size = len(bytes)
        
        filename = os.path.basename(filename)
        sanfilename = filename.replace('.','')
        
        print("")
        output = "const char %s[%d] = {\n" % (sanfilename, size)

        pos = 0
        size = len(bytes)
        for byte in bytes:
                if (pos % 11) == 0:
                        output += "        "
                output += "0x%02x" % (byte)
                if (pos + 1) == size:
                        output += '\n'
                elif (pos % 11) == 10:
                        output += ',\n'
                elif (pos + 1) < size:
                        output += ", "
                pos += 1

        output += "};"

        print(output)
        
# Finally, print the struct that contains all md filenames
# and char array names.
print("")
print("const struct help_file help_files[] = {")

for filename in sys.argv[1:]:
        filename = os.path.basename(filename)
        sanfilename = filename.replace('.','')
        print('        {"'+str(filename)+str('",'),sanfilename+str('},'))
       
print("        {NULL, NULL}")
print("};")