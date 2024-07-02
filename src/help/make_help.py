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

import json
import os
import re
import sys

print("// This file is auto-generated.")
print("#include <stdlib.h>")
print('#include "help_text.h"')

help_files = []
for filename in sys.argv[1:]:
	with open(filename, "r") as f:
		text = f.read()

	help_files.append((os.path.basename(filename), text))

print("const struct help_file help_files[] = {\n%s\t{NULL, NULL}\n};" % (
	"".join("\t{%s, %s},\n" % (json.dumps(filename), json.dumps(text))
	        for filename, text in help_files),
))
