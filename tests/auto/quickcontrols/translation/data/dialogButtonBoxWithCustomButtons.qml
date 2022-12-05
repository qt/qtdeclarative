// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    property Dialog dialog: Dialog {
        width: 300
        height: 300
        visible: true

        footer: DialogButtonBox {
            Button {
                objectName: "okButton"
                text: qsTr("OK")

                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                objectName: "cancelButton"
                text: qsTr("Cancel")

                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }
    }
}
