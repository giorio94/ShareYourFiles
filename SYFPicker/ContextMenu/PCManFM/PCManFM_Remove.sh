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


# Build the variables
FILEPATH="$HOME/.local/share/file-manager/actions"
ICONPATH="$HOME/.local/share/icons/hicolor/48x48/apps"
FILE="$FILEPATH/SYFPicker.desktop"
ICON="$ICONPATH/ShareYourFiles.png"

# Remove the file
rm -f $FILE || \
{ echo "Failed removing the desktop file. Exit" ; exit 1; }

# Remove the icon
rm -f $ICON || \
{ echo "Failed removing the icon. Exit" ; exit 1; }

echo "Desktop file removed correctly"
echo "Remember to restart the file manager"
