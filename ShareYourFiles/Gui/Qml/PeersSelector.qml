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

    width: 775
    height: 550

    minimumWidth: 600
    minimumHeight: 400

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    visible: true
    title: qsTr("Share Your Files - Select the recipients")

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    font.family: appFont.name

    Material.theme: Material.Dark
    Material.accent: Material.Green

    property bool confirmed: false
    property bool empty: peersSelector.rowCount === 0
    property bool selected: peersSelector.selectedCount > 0

    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 75

            Label {
                property string files: (peersSelector.filesNumber === 1)
                                       ? qsTr("file")
                                       : qsTr("files")
                text: emph(peersSelector.filesNumber) +" " + files +
                      qsTr(" selected for transfer ") +
                      emph("(" + peersSelector.filesSize + ")")

                fontSizeMode: Text.Fit
                font.pointSize: 35
                minimumPointSize: 10

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 300

            Item {
                anchors.fill: parent
                visible: root.empty

                Label {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    property string accentHex: Material.accent
                    text: qsTr("No other") + emph(qsTr(" users ")) +
                          qsTr("currently") + emph(qsTr(" online"))
                    font.pointSize: 30
                }
            }

            GridView {
                id: peerView

                anchors.fill: parent
                visible: !root.empty

                cellWidth: 180
                cellHeight: 220

                clip: true

                ScrollBar.vertical: ScrollBar { }

                ToolTip.visible: top && !empty
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Select the users you want to share the files with by clicking on them.")

                model: peersSelector

                delegate: Item {
                    width: peerView.cellWidth
                    height: peerView.cellHeight

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10

                        CircularImage {
                            Layout.alignment: Qt.AlignCenter

                            imageSet: model.iconSet
                            imagePath: "file:///" + model.iconPath

                            borderSet: model.selected
                            borderColor: Material.accent
                        }

                        Label {
                            text: model.firstName
                            color: Material.accent

                            Layout.fillWidth: true
                            fontSizeMode: Text.HorizontalFit
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Label {
                            text: model.lastName

                            Layout.fillWidth: true
                            fontSizeMode: Text.HorizontalFit
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    MouseArea {
                        anchors.centerIn: parent
                        anchors.fill: parent
                        onClicked: {
                            peersSelector.toggleSelected(index);
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 100
            Layout.maximumHeight: 150
            Layout.topMargin: 10

            Flickable {
                id: messageFlickable
                anchors.fill: parent
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { }

                TextArea.flickable: TextArea {
                    id: message
                    height: messageFlickable.height
                    wrapMode: TextArea.Wrap
                    padding: 10

                    focus: true

                    property int maximumLength: 500
                    property string previousText: text

                    placeholderText: qsTr("Enter a message to the recipients (max ")
                                     + maximumLength + qsTr(" characters)...");
                    font.pointSize: 11

                    selectByMouse: true

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

        DialogButtonBox {
            Layout.fillWidth: true
            spacing: 20

            background: Rectangle { color: "transparent" }

            Button {
                width: 100
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: root.close()

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Cancel the file sharing.")
            }

            Button {
                width: 100
                text: qsTr("Share")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                enabled: root.selected
                onClicked: {
                    root.confirmed = true
                    root.close()
                }

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Share the files with the selected users.")
            }
        }
    }

    onClosing: {
        if (root.confirmed) {
            peersSelector.setMessage(message.text)
        }

        peersSelector.completeSelection(root.confirmed)
    }
}
