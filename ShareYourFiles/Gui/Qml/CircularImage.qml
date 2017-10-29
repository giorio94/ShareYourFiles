/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtGraphicalEffects 1.0

Rectangle {
    property int imageSize: 128

    property bool imageSet: false
    property string imagePath: ""
    property string imageDefaultPath: "qrc:/Resources/NoImage.jpg"

    property bool borderSet: false
    property int borderSize: 5
    property string borderColor: "white"

    id: root

    width: imageSize + borderSize * 2
    height: imageSize + borderSize * 2
    radius: imageSize + borderSize * 2
    color: borderSet
           ? borderColor
           : "transparent"

    Image {
        id: image

        anchors.centerIn: parent
        source: root.imageSet
                ? root.imagePath
                : root.imageDefaultPath

        width: root.imageSize
        height: root.imageSize

        sourceSize.width: root.imageSize
        sourceSize.height: root.imageSize

        fillMode: Image.PreserveAspectCrop
        cache: false

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                width: root.imageSize
                height: root.imageSize
                radius: root.imageSize
            }
        }
    }
}
