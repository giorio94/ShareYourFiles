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

    width: 700
    height: 550

    minimumWidth: 700
    minimumHeight: 550

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    visible: true
    title: qsTr("Share Your Files - existing file detected")

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    font.family: appFont.name

    modality: Qt.WindowModal

    Material.theme: Material.Dark
    Material.accent: Material.Green

    property var request: undefined
    property bool selfDestroy: false

    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: 50

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            text: qsTr("There is already a file with the same name in this location")

            fontSizeMode: Text.Fit
            font.pointSize: 25
            color: Material.accent
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Name: ") + emph(request.filename)

            elide: Text.ElideMiddle
            font.pointSize: 14
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Path: ") + emph(request.filepath)

            elide: Text.ElideMiddle
            font.pointSize: 12
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 20

            Layout.minimumHeight: 100
            Layout.maximumHeight: 125

            label: Text {
                text: qsTr("Existing file:")
                color: Material.accent
            }

            ColumnLayout {
                anchors.fill: parent

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Size: ") + emph(request.oldFileSize)
                    font.pointSize: 12
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Date modified: ") + emph(request.oldFileDate)
                    font.pointSize: 12
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 25
            Layout.minimumHeight: 125
            Layout.maximumHeight: 150

            label: Text {
                text: qsTr("Received file:")
                color: Material.accent
            }

            ColumnLayout {
                anchors.fill: parent

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Sender: ") + emph(request.names)
                    font.pointSize: 12
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Size: ") + emph(request.newFileSize)
                    font.pointSize: 12
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Date modified: ") + emph(request.newFileDate)
                    font.pointSize: 12
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 15

            CheckBox {
                id: checkBoxAlways
                text: qsTr("Always for this reception")
                checked: false

                contentItem: Text {
                    text: checkBoxAlways.text
                    color: Material.accent
                    font.pointSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: checkBoxAlways.indicator.width + checkBoxAlways.spacing
                }
            }

            DialogButtonBox {
                Layout.fillWidth: true
                spacing: 20

                background: Rectangle { color: "transparent" }

                Button {
                    width: 100
                    text: qsTr("Keep both")
                    DialogButtonBox.buttonRole: DialogButtonBox.RejectRole

                    onClicked: {
                        request.keepBoth(checkBoxAlways.checked)
                        root.close();
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.text: qsTr("Keep the existing file and add a suffix to the received one.")
                }

                Button {
                    width: 100
                    text: qsTr("Keep")
                    DialogButtonBox.buttonRole: DialogButtonBox.RejectRole

                    onClicked: {
                        request.keepExisting(checkBoxAlways.checked)
                        root.close();
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.text: qsTr("Keep the existing file and discard the received one.")
                }

                Button {
                    width: 100
                    text: qsTr("Replace")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

                    onClicked: {
                        request.replaceExisting(checkBoxAlways.checked)
                        root.close();
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.text: qsTr("Replace the existing file with the received one.")
                }
            }
        }
    }

    Connections {
        target: request
        onConnectionAborted: root.close()
    }

    onClosing: {
        if (root.selfDestroy) {
            root.destroy()
        }
    }

    Component.onDestruction: {
        if (root.selfDestroy) {
            request.requestDestruction()
        }
    }
}
