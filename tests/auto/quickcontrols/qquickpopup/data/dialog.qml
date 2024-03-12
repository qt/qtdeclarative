// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Item {
    width: 400
    height: 400
    objectName: "Rectangle"

    property alias dialog: dialog

    Dialog {
        id: dialog
        objectName: "Dialog"
        width: 200
        height: 200
        anchors.centerIn: parent
        visible: true

        Component.onCompleted: {
            background.objectName = "DialogBackground"
            contentItem.objectName = "DialogContentItem"
        }
    }
}
