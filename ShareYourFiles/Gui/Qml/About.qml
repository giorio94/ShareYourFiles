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

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

ApplicationWindow {
    id: root

    width: 675
    height: 675

    minimumWidth: 675
    minimumHeight: 675
    maximumWidth: 675
    maximumHeight: 675

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    title: qsTr("Share Your Files - About")

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    font.family: appFont.name

    Material.theme: Material.Dark
    Material.accent: Material.Green

    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent

        Image {
            Layout.topMargin: 10
            Layout.alignment: Qt.AlignHCenter

            width: 100
            height: 100
            sourceSize.width: 100
            sourceSize.height: 100
            source: "qrc:/Resources/IconGreen.svg"
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter

            text: qsTr("Share Your Files (SYF)")
            font.pointSize: 30
            color: Material.accent
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter

            text: qsTr("Peer to peer") + emph(" \u2014 ") + qsTr("on LAN") +
                  emph(" \u2014 ") + qsTr("file sharing")
            font.pointSize: 16
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: -5
            horizontalAlignment: Text.AlignHCenter

            text: qsTr("Version: ") + emph(version)
            font.pointSize: 14
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 10
            Layout.bottomMargin: 5
            horizontalAlignment: Text.AlignHCenter

            text: "Copyright \u00A9 2017" + emph(" Marco Iorio ") +
                  "(giorio94 at gmail dot com)"
            font.pointSize: 16
        }

        TextArea {
            Layout.minimumWidth: 550
            Layout.maximumWidth: 550
            Layout.alignment: Qt.AlignCenter
            horizontalAlignment: Text.AlignHCenter

            readOnly: true
            activeFocusOnPress: false
            activeFocusOnTab: false

            padding: 10
            wrapMode: TextArea.Wrap

            font.pointSize: 10
            text: license
        }

        Label {
            text: qsTr("External components:")
            color: Material.accent

            Layout.fillWidth: true
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 14
        }

        Label {
            text: emph("CuteLogger ") + "(http://github.com/dept2/CuteLogger)" +
                  " licensed under " + emph("LGPL 2.1")

            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
        }
        Label {
            text: emph("Source Sans Pro") + qsTr(" font by ") + emph("Paul D. Hunt") +
                  " licensed under " + emph("OFL 1.1")

            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
        }
        Label {
            text: qsTr("SYF icon by ") + emph("Washaweb") +
                  " (http://www.washaweb.com) " + " licensed under " +
                  emph("CC BY 3.0")

            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
        }
        Label {
            text: qsTr("Other icons by ") + emph("Austin Andrews") +
                  " (http://materialdesignicons.com) " + " licensed under " +
                  emph("OFL 1.1")

            Layout.fillWidth: true
            Layout.bottomMargin: 10
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 10
        }

        DialogButtonBox {
            Layout.fillWidth: true

            background: Rectangle { color: "transparent" }

            Button {
                width: 100
                text: qsTr("Ok")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: root.close()
            }
        }
    }

    property string license:
        qsTr("SYF is free software: you can redistribute it and/or modify ") +
        qsTr("it under the terms of the GNU General Public License as published by ") +
        qsTr("the Free Software Foundation, either version 3 of the License, or ") +
        qsTr("(at your option) any later version.\n\n") +
        qsTr("SYF is distributed in the hope that it will be useful, ") +
        qsTr("but WITHOUT ANY WARRANTY; without even the implied warranty of ") +
        qsTr("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the ") +
        qsTr("GNU General Public License for more details.\n\n") +
        qsTr("You should have received a copy of the GNU General Public License ") +
        qsTr("along with SYF.  If not, see <http://www.gnu.org/licenses/>.");
}
