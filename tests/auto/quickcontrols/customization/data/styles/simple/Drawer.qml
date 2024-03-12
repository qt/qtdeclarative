// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Drawer {
    id: control
    objectName: "drawer-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Item {
        objectName: "drawer-contentItem-simple"
    }

    background: Rectangle {
        objectName: "drawer-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
