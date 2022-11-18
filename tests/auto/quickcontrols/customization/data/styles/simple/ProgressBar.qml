// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ProgressBar {
    id: control
    objectName: "progressbar-simple"

    implicitWidth: Math.max(contentItem.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, background.implicitHeight)

    contentItem: Item {
        objectName: "progressbar-contentItem-simple"
    }

    background: Rectangle {
        objectName: "progressbar-background-simple"
        color: control.indeterminate ? "red" : "green"
    }
}
