// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "ComboBox is a combined button and popup list. It presents "
                + "a list of options to the user that occupies minimal screen space."
        }

        ComboBox {
            model: ["First", "Second", "Third"]
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "ComboBox can be made \l editable. An editable combo box auto-"
                + "completes its text based on what is available in the model."
        }

        ComboBox {
            id: comboBox

            editable: true
            model: ListModel {
                ListElement { text: "Banana" }
                ListElement { text: "Apple" }
                ListElement { text: "Coconut" }
            }
            onAccepted: {
                if (find(editText) === -1)
                    comboBox.model.append({text: comboBox.editText})
            }
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
