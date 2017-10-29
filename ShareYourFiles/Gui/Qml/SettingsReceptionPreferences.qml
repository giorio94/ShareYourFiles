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

Item {
    id: root
    property var model: undefined

    property string accentHex: Material.accent
    function emph(text) {
        return "<font color=\"" + accentHex + "\">" + text + "</font>";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.centerIn: parent

        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            Label {
                Layout.rightMargin: 15
                text: qsTr("When a new request is received:")
            }
            ComboBox {
                id: action
                model: root.model.actionValues()
                currentIndex: root.model.action

                Binding {
                    target: root.model
                    property: "action"
                    value: action.currentIndex
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.topMargin: 5
            Layout.bottomMargin: 10

            Label {
                anchors.horizontalCenter: parent.horizontalCenter

                text: qsTr("Save the received files in:")
                color: Material.accent
            }
        }

        RowLayout {
            TextField {
                id: dataPath

                Layout.fillWidth: true
                Layout.rightMargin: 20
                placeholderText: qsTr("Path")
                text: model.dataPath
                selectByMouse: true
                font.pointSize: 12

                Binding {
                    target: model
                    property: "dataPath"
                    value: dataPath.text
                }
            }
            Button {
                text: qsTr("Change")
                onClicked: pathSelector.open()
            }
        }

        CheckBox {
            id: folderUser
            text: qsTr("Create a folder for each sender")
            Layout.topMargin: -10
            Layout.bottomMargin: -25
            checked: model.folderUser

            Binding {
                target: model
                property: "folderUser"
                value: folderUser.checked
            }
        }

        CheckBox {
            id: folderDate
            text: qsTr("Create a folder with the date of the transfer")
            checked: model.folderDate

            Binding {
                target: model
                property: "folderDate"
                value: folderDate.checked
            }
        }

        Switch {
            id: overwrite
            text: emph(qsTr("Overwrite")) + qsTr(" user specific preferences");
            Layout.topMargin: -15
            font.pointSize: 12
            checked: model.overwrite
            anchors.horizontalCenter: parent.horizontalCenter

            Binding {
                target: model
                property: "overwrite"
                value: overwrite.checked
            }
        }
    }

    FileDialog {
        id: pathSelector
        title: "Select the folder where the received files will be stored"
        folder: shortcuts.documents
        selectFolder: true

        onAccepted: {
            model.setDataPathUrl(pathSelector.fileUrl)
        }
    }
}
