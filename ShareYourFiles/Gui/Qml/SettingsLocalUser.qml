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

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 20

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter

                TextField {
                    id: firstName

                    Layout.fillWidth: true
                    Layout.leftMargin: 5
                    Layout.rightMargin: 15

                    placeholderText: "First name"
                    text: model.firstName
                    maximumLength: 16
                    selectByMouse: true

                    Binding {
                        target: model
                        property: "firstName"
                        value: firstName.text
                    }
                }

                TextField {
                    id: lastName

                    Layout.fillWidth: true
                    Layout.leftMargin: 5
                    Layout.rightMargin: 15

                    placeholderText: "Last name"
                    text: model.lastName
                    maximumLength: 16
                    selectByMouse: true

                    Binding {
                        target: model
                        property: "lastName"
                        value: lastName.text
                    }
                }
            }

            CircularImage {
                id: userIcon

                imageSize: 128
                imageSet: model.iconSet
                imagePath: "file:///" + model.iconPath
                borderSet: true
                borderColor: Material.accent
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: -10
            spacing: 35

            Button {
                text: qsTr("Clear image")
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                width: 125
                onClicked: model.iconSet = false
            }
            Button {
                text: qsTr("Choose image")
                DialogButtonBox.buttonRole: DialogButtonBox.Apply
                width: 125
                onClicked: iconPicker.open()
            }
        }

        FileDialog {
            id: iconPicker
            title: "Select an image"
            folder: shortcuts.pictures

            nameFilters: ["Image files (*.jpg *.jpeg *.png *.bmp *.gif)"]

            onAccepted: {
                model.iconPath = iconPicker.fileUrl
                model.iconSet = true
            }
        }
    }
}
