// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ToolBar {
    id: control
    objectName: "toolbar-simple"

    implicitWidth: Math.max(background.implicitWidth, contentWidth)
    implicitHeight: Math.max(background.implicitHeight, contentHeight)

    contentWidth: contentItem.implicitWidth || (contentChildren.length === 1 ? contentChildren[0].implicitWidth : 0)
    contentHeight: contentItem.implicitHeight || (contentChildren.length === 1 ? contentChildren[0].implicitHeight : 0)

    contentItem: Item {
        objectName: "toolbar-contentItem-simple"
    }

    background: Rectangle {
        objectName: "toolbar-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
