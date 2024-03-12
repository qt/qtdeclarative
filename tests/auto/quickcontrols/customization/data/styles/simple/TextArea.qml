// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.TextArea {
    id: control
    objectName: "textarea-simple"

    implicitWidth: Math.max(contentWidth, background.implicitWidth)
    implicitHeight: Math.max(contentHeight, background.implicitHeight)

    background: Rectangle {
        objectName: "textarea-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }
}
