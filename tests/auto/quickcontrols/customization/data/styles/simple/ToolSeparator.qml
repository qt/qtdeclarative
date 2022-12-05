// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ToolSeparator {
    id: control
    objectName: "toolseparator-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Text {
        objectName: "toolseparator-contentItem-simple"
    }

    background: Rectangle {
        objectName: "toolseparator-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
