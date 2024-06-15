#!/bin/sh

path=$(dirname "$0")

# If this script is running, the user has granted permission for this
# app to run. We need to pass that permission to the main executable
# before we launch it in the terminal, otherwise the same permission
# error will occur again.
"$path/wadgadget" --version

open "$path/wadgadget"
