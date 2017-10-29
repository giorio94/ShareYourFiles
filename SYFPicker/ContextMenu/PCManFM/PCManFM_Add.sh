#!/bin/bash

# Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
# This file is part of Share Your Files (SYF).

# SYF is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# SYF is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with SYF.  If not, see <http://www.gnu.org/licenses/>.


# Check if the parameter is provided
if [ $# -ne 1 ];
    then echo "Usage: $0 path/to/SYFPicker"
    exit 1;
fi

SYFPICKER="$(cd "$(dirname "$1")"; pwd)/$(basename "$1")"
if [ ! -x "$SYFPICKER" ]; then
	echo "Specified file does not exist or is not executable. Exit";
	exit 1;
fi

# Build the variables
FILEPATH="$HOME/.local/share/file-manager/actions"
ICONPATH="$HOME/.local/share/icons/hicolor/48x48/apps"
FILE="$FILEPATH/SYFPicker.desktop"
ICONNAME="ShareYourFiles"

FILE_CONTENT=\
"[Desktop Entry]
Name = Send with SYF
Tooltip = Send the selected files and folders with Share Your Files
Icon = $ICONNAME
Profiles = profile-zero;

[X-Action-Profile profile-zero]
Name = Send with SYF
Exec = $SYFPICKER %F"

# Create the directory if not already exists
mkdir -p $FILEPATH || \
{ echo "Failed creating $FILEPATH. Exit"; exit 1; }

# Write the file content
echo "$FILE_CONTENT" > $FILE || \
{ echo "Failed creating the desktop file. Exit" ; exit 1; }

# Create the directory if not already exists
mkdir -p $ICONPATH || \
{ echo "Failed creating $ICONPATH. Exit"; exit 1; }

# Copy the icon
cp "$ICONNAME.png" "$ICONPATH/$ICONNAME.png" || \
{ echo "Failed copying the icon. Exit"; exit 1; }

echo "Desktop file created correctly"
echo "Remember to restart the file manager"
