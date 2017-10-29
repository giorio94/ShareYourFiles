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

    width: 400
    height: 675

    minimumWidth: 400
    minimumHeight: 650

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    title: qsTr("Share Your Files - Settings")

    FontLoader { id: appFont; source: "qrc:/Resources/SourceSansPro.ttf" }
    font.family: appFont.name

    Material.theme: Material.Dark
    Material.accent: Material.Green

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 20

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.bottomMargin: 20

            Label {
                text: "Settings"

                fontSizeMode: Text.HorizontalFit
                font.pointSize: 40
                minimumPointSize: 10
                color: Material.accent

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        }

        SettingsLocalUser {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
            Layout.minimumHeight: 250
            Layout.preferredHeight: 250
            Layout.maximumHeight: 250

            model: settings
        }

        Rectangle {
            height: 1
            Layout.fillWidth: true
            color: Material.accent
        }

        SettingsReceptionPreferences {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
            Layout.topMargin: 5
            Layout.minimumHeight: 225
            Layout.preferredHeight: 250
            Layout.maximumHeight: 325

            model: settings
        }

        DialogButtonBox {
            id: buttons

            Layout.fillWidth: true
            spacing: 20

            background: Rectangle {
                color: "transparent"
            }

            Button {
                width: 100
                text: qsTr("Reset")
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                enabled: settings.modified;
                onClicked: settings.resetChanges()

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Reset the previous settings.")
            }

            Button {
                width: 100
                text: qsTr("Save")
                DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
                enabled: settings.modified;
                onClicked: saveChanges()

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Save the new settings.")
            }

            Button {
                width: 100
                text: qsTr("Hide")
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: root.close()

                ToolTip.visible: hovered
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: qsTr("Hide this window.")
            }
        }
    }

    MessageDialog {
        id: emptyNamesDialog
        title: root.title
        text: qsTr("You need to specify at least one name.")
        icon: StandardIcon.Warning
        standardButtons: StandardButton.Ok
    }

    MessageDialog {
        id: saveErrorDialog
        title: root.title
        text: qsTr("An error occurred while saving your preferences.\nSome changes may have been lost.")
        icon: StandardIcon.Critical
        standardButtons: StandardButton.Ok
    }

    MessageDialog {
        id: askConfirmationDialog
        title: root.title
        text: qsTr("Some preferences have been changed.\nWould you like to save them?")
        icon: StandardIcon.Question
        standardButtons: StandardButton.Yes | StandardButton.No | StandardButton.Cancel

        onYes: {
            saveChanges()
            root.close()
        }
        onNo: {
            settings.resetChanges()
            root.close()
        }
    }

    onClosing: {
        if (settings.modified) {
            close.accepted = false
            askConfirmationDialog.open()
        }
    }

    function saveChanges() {
        // Check if both names are empty
        if ((/^\s*$/).test(settings.firstName) &&
                (/^\s*$/).test(settings.lastName)) {

            emptyNamesDialog.open();
            return;
        }

        // Try to save che changes
        if (!settings.saveChanges()) {
            saveErrorDialog.open()
        }
    }
}
