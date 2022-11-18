// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Popup {
    id: control
    objectName: "popup-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Item {
        objectName: "popup-contentItem-simple"
    }

    background: Rectangle {
        objectName: "popup-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
