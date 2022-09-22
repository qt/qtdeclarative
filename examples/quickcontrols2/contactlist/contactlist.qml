// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property int currentContact: -1

    width: 320
    height: 480
    visible: true
    title: qsTr("Contact List")

    ContactDialog {
        id: contactDialog
        onFinished: function(fullName, address, city, number) {
            if (currentContact === -1)
                contactView.model.append(fullName, address, city, number)
            else
                contactView.model.set(currentContact, fullName, address, city, number)
        }
    }

    Menu {
        id: contactMenu
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
        modal: true

        Label {
            padding: 10
            font.bold: true
            width: parent.width
            horizontalAlignment: Qt.AlignHCenter
            text: currentContact >= 0 ? contactView.model.get(currentContact).fullName : ""
        }
        MenuItem {
            text: qsTr("Edit...")
            onTriggered: contactDialog.editContact(contactView.model.get(currentContact))
        }
        MenuItem {
            text: qsTr("Remove")
            onTriggered: contactView.model.remove(currentContact)
        }
    }

    ContactView {
        id: contactView
        anchors.fill: parent
        onPressAndHold: {
            currentContact = index
            contactMenu.open()
        }
    }

    RoundButton {
        text: qsTr("+")
        highlighted: true
        anchors.margins: 10
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        onClicked: {
            currentContact = -1
            contactDialog.createContact()
        }
    }
}
