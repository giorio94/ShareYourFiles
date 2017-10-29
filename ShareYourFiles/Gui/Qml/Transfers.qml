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

    width: 800
    height: 700

    minimumWidth: 450
    minimumHeight: 450

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    title: qsTr("Share Your Files - Transfers")

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    FontLoader { id: iconsFont; source: "qrc:/Resources/FontIcons.ttf" }
    font.family: appFont.name

    Material.theme: Material.Dark
    Material.accent: Material.Green

    property bool empty: transfers.rowCount === 0

    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.bottomMargin: 40

            Label {
                text: "Transfers"

                fontSizeMode: Text.HorizontalFit
                font.pointSize: 40
                minimumPointSize: 10
                color: Material.accent

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                anchors.fill: parent
                visible: root.empty

                Label {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    property string accentHex: Material.accent
                    text: qsTr("No") + emph(qsTr(" started ")) +
                          qsTr("transfers")
                    font.pointSize: 30
                }
            }

            ListView {
                id: transfersView

                anchors.fill: parent
                visible: !root.empty
                clip: true

                ScrollBar.vertical: ScrollBar { }

                model: transfers

                delegate: Item {
                    width: transfersView.width
                    height: 250

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 25
                        anchors.rightMargin: 25

                        CircularImage {

                            imageSet: model.iconSet
                            imagePath: "file:///" + model.iconPath

                            borderSet: true
                            borderColor: Material.accent
                        }


                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.leftMargin: 25
                            Layout.alignment: Qt.AlignLeft

                            Label {
                                property string transfer: model.sender ?
                                                              qsTr("Transfer to ") :
                                                              qsTr("Reception from ")
                                property string files: (model.totalNumber === 1)
                                                       ? qsTr("file")
                                                       : qsTr("files")

                                text: transfer + emph(model.names) + " ("
                                      + model.totalNumber + " " + files + " - "
                                      + model.totalSize + ")"

                                Layout.fillWidth: true
                                Layout.bottomMargin: -10
                                elide: Text.ElideRight
                                font.pointSize: 15
                            }

                            RowLayout {
                                Label {
                                    property string complete: model.inTransfer
                                                              ? " (" + model.percentage +
                                                                "% " + qsTr("complete") + ")"
                                                              : ""
                                    text: qsTr("Status: ") + emph(model.status) + complete

                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    font.pointSize: 13
                                }

                                ToolButton {
                                    id: buttonPause

                                    font.family: iconsFont.name
                                    font.pointSize: 15
                                    text: "\uf278"

                                    Layout.alignment: Qt.AlignRight
                                    Layout.rightMargin: -20
                                    visible: !model.closed
                                    checkable: true
                                    checked: model.paused
                                    onClicked: transfers.pauseConnection(index,
                                                                         buttonPause.checked);
                                }
                                ToolButton {
                                    font.family: iconsFont.name
                                    font.pointSize: 15
                                    text: "\uf2c2"

                                    Layout.alignment: Qt.AlignRight
                                    visible: !model.closed
                                    onClicked: {
                                        abortDialog.sender = model.sender
                                        abortDialog.names = model.names
                                        abortDialog.index = index
                                        abortDialog.open()
                                    }
                                }
                                ToolButton {
                                    font.family: iconsFont.name
                                    font.pointSize: 15
                                    text: "\uf1a4"

                                    Layout.alignment: Qt.AlignRight
                                    visible: model.closed
                                    onClicked: transfers.deleteConnection(index);
                                }
                            }

                            ProgressBar {
                                Layout.fillWidth: true
                                Layout.topMargin: 5
                                Layout.bottomMargin: 15

                                from: 0
                                to: 100
                                value: model.percentage
                            }

                            Label {
                                text: qsTr("File in transfer: ") +
                                      emph(model.filename==="" ?
                                               qsTr("none") : model.filename)

                                Layout.fillWidth: true
                                elide: Text.ElideMiddle
                                font.pointSize: 11
                            }

                            Label {
                                text: qsTr("Current speed: ") + emph(model.speed) +
                                      qsTr(" (average: ") + emph(model.avgSpeed) + ")"

                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.pointSize: 11
                            }

                            Label {
                                text: qsTr("Remaining time: ") +
                                      emph(model.inTransfer
                                           ? (model.remainingTime === ""
                                              ? qsTr("unknown")
                                              : qsTr("about ") + model.remainingTime)
                                           : qsTr("not in transfer"))

                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.pointSize: 11
                            }
                            Label {
                                text: qsTr("Remaining items: ") +
                                      emph(model.remainingNumber + " (" +
                                           model.remainingSize + ")")

                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.pointSize: 11
                            }
                            Label {
                                text: qsTr("Skipped or rejected items: ") +
                                      emph(model.skippedNumber + " (" +
                                           model.skippedSize + ")")

                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.pointSize: 11
                            }
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Material.accent
                    }
                }
            }
        }

        DialogButtonBox {
            id: buttons

            Layout.fillWidth: true

            background: Rectangle {
                color: "transparent"
            }

            Button {
                width: 100
                text: qsTr("Hide")
                onClicked: root.close()

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Hide this window.")
            }
        }

        MessageDialog {
            id: abortDialog

            property bool sender: true
            property string names: ""
            property int index: 0

            title: root.title
            text: qsTr("Abort the") +
                  (sender ? qsTr(" transfer to ") : qsTr(" reception from ")) +
                  names + "?";

            icon: StandardIcon.Question
            standardButtons: StandardButton.Yes | StandardButton.No

            onYes: transfers.abortConnection(index)
        }

        Connections {
            target: transfers
            onTransferRequestedAsk: {
                var component = Qt.createComponent("TransferRequest.qml");
                var object = component.createObject(root, {"request": request, "selfDestroy": true});
            }
            onTransferResponseReceived: {
                var component = Qt.createComponent("TransferResponse.qml");
                var object = component.createObject(root, {"response": response, "selfDestroy": true});
            }
            onDuplicatedFileDetected: {
                var component = Qt.createComponent("DuplicatedFile.qml");
                var object = component.createObject(root, {"request": request, "selfDestroy": true});
            }
        }
    }
}
