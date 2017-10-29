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

    width: 700
    height: 700

    minimumWidth: 550
    minimumHeight: 600

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    visible: true
    title: qsTr("Share Your Files - Transfer request received")

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

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: 20

                Label {
                    property string files: (request.filesNumber === 1)
                                           ? qsTr("file")
                                           : qsTr("files")

                    text: emph(request.names) +
                          qsTr(" would like to share with you ") +
                          emph(request.filesNumber) + " " + files +
                          emph(" (" + request.filesSize + ")")

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

                imageSet: request.iconSet
                imagePath: "file:///" + request.iconPath
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
                text: qsTr("Message from ") + request.names + ":"
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
                    text: request.message

                    font.pointSize: 11
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 75
            Layout.preferredHeight: 125
            Layout.topMargin: 10

            Flickable {

                id: messageFlickable
                anchors.fill: parent
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { }


                TextArea.flickable: TextArea {
                    id: message
                    height: messageFlickable.height

                    padding: 10
                    wrapMode: TextArea.Wrap

                    selectByMouse: true
                    focus: true

                    placeholderText: qsTr("Enter a message to the sender (max ")
                                     + maximumLength + qsTr(" characters)...");
                    font.pointSize: 11

                    property int maximumLength: 500
                    property string previousText: text
                    onTextChanged: {
                        if (text.length > maximumLength) {
                            var cursor = cursorPosition;
                            text = previousText;
                            if (cursor > text.length) {
                                cursorPosition = text.length;
                            } else {
                                cursorPosition = cursor-1;
                            }
                        }

                        previousText = text
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160

            label: Text {
                text: qsTr("Save the files in:")
                color: Material.accent
                leftPadding: 5
            }
            padding: 10
            background: Item { }

            ColumnLayout {

                anchors.fill: parent
                anchors.topMargin: -15

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextField {
                        id: dataPath

                        Layout.fillWidth: true
                        Layout.rightMargin: 20
                        placeholderText: qsTr("Path")
                        text: request.dataPath
                        selectByMouse: true
                    }
                    Button {
                        text: qsTr("Change")
                        onClicked: pathSelector.open()
                    }
                }

                CheckBox {
                    id: folderUser
                    text: qsTr("Create a folder with the name of the sender")
                    checked: request.folderUser
                    Layout.fillWidth: true
                    Layout.topMargin: -10
                    Layout.bottomMargin: -25
                }

                CheckBox {
                    id: folderDate
                    text: "Create a folder with the date of the transfer"
                    checked: request.folderDate
                    Layout.fillWidth: true
                }

            }
        }

        RowLayout {
            Layout.fillWidth: true

            CheckBox {
                id: checkBoxAlways
                text: qsTr("Always for this user")
                checked: false
                visible: !request.anonymous

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
                    text: qsTr("Reject")
                    DialogButtonBox.buttonRole: DialogButtonBox.RejectRole

                    onClicked: {
                        request.reject(message.text, checkBoxAlways.checked);
                        root.close();
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.text: qsTr("Reject the file sharing.")
                }

                Button {
                    width: 100
                    text: qsTr("Accept")
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

                    onClicked: {
                        request.accept(dataPath.text, folderUser.checked,
                                       folderDate.checked, message.text,
                                       checkBoxAlways.checked);
                        root.close();
                    }

                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.text: qsTr("Accept the file sharing.")
                }
            }
        }
    }

    FileDialog {
        id: pathSelector
        title: "Select the folder where the received files will be stored"
        folder: shortcuts.documents
        selectFolder: true

        onAccepted: {
            dataPath.text = request.urlToPath(pathSelector.fileUrl)
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
