// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.TabBar {
    id: control
    objectName: "tabbar-simple"

    implicitWidth: Math.max(background.implicitWidth, contentItem.implicitWidth)
    implicitHeight: Math.max(background.implicitHeight, contentItem.implicitHeight)

    contentItem: Item {
        objectName: "tabbar-contentItem-simple"
    }

    background: Rectangle {
        objectName: "tabbar-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
