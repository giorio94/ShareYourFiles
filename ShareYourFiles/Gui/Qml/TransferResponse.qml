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
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0

ApplicationWindow {
    id: root

    width: 600
    height: 400

    minimumWidth: 500
    minimumHeight: 400

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    visible: true
    title: qsTr("Share Your Files - Transfer request ") + answer

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    font.family: appFont.name

    modality: Qt.WindowModal

    Material.theme: Material.Dark
    Material.accent: Material.Green

    property var response: undefined
    property bool selfDestroy: false
    property string answer: response.accepted ? qsTr("accepted") : qsTr("rejected")


    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: 20

                Label {
                    text: emph(response.names) + " " + answer +
                          qsTr(" your sharing request");

                    wrapMode: Text.WordWrap
                    fontSizeMode: Text.Fit
                    font.pointSize: 25
                    minimumPointSize: 10

                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            CircularImage {
                imageSize: 128

                imageSet: response.iconSet
                imagePath: "file:///" + response.iconPath
                borderSet: true
                borderColor: Material.accent
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            Layout.preferredHeight: 150
            Layout.topMargin: 10

            label: Text {
                text: qsTr("Message from ") + response.names + ":"
                color: Material.accent
                leftPadding: 5
            }
            padding: 0
            background: Item { }

            Flickable {
                id: peerMessageFlickable
                anchors.fill: parent
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { }

                TextArea.flickable: TextArea {
                    height: peerMessageFlickable.height
                    padding: 10
                    wrapMode: TextArea.Wrap

                    readOnly: true
                    activeFocusOnPress: false
                    activeFocusOnTab: false
                    selectByMouse: true

                    placeholderText: qsTr("No message...");
                    text: response.message

                    font.pointSize: 11
                }
            }
        }

        DialogButtonBox {
            Layout.fillWidth: true
            background: Rectangle { color: "transparent" }

            Button {
                width: 100
                text: qsTr("Ok")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

                onClicked: root.close();
            }
        }
    }

    onClosing: {
        if (root.selfDestroy) {
            root.destroy()
        }
    }

    Component.onDestruction: {
        if (root.selfDestroy) {
            response.requestDestruction()
        }
    }
}
