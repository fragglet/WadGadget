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

EXTN_RE = re.compile(r"\..*$")

print("// This file is auto-generated.")

for filename in sys.argv[1:]:
	with open(filename, "r") as f:
		text = f.read()

	stem = EXTN_RE.sub("", os.path.basename(filename))
	print("const char *%s_text = %s;" % (stem, json.dumps(text)))
