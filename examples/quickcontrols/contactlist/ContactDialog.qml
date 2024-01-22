// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog

    signal finished(string fullName, string address, string city, string number)

    function createContact() {
        form.fullName.clear();
        form.address.clear();
        form.city.clear();
        form.number.clear();

        dialog.title = qsTr("Add Contact");
        dialog.open();
    }

    function editContact(contact) {
        form.fullName.text = contact.fullName;
        form.address.text = contact.address;
        form.city.text = contact.city;
        form.number.text = contact.number;

        dialog.title = qsTr("Edit Contact");
        dialog.open();
    }

    x: parent.width / 2 - width / 2
    y: parent.height / 2 - height / 2

    focus: true
    modal: true
    title: qsTr("Add Contact")
    standardButtons: Dialog.Ok | Dialog.Cancel

    contentItem: ContactForm {
        id: form
    }

    onAccepted: {
        if (form.fullName.text && form.address.text && form.city.text && form.number.text) {
            finished(form.fullName.text, form.address.text, form.city.text, form.number.text);
        }
    }
}
