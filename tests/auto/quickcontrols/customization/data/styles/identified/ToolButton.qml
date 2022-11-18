// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ToolButton {
    id: control
    objectName: "toolbutton-identified"

    contentItem: Item {
        id: contentItem
        objectName: "toolbutton-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "toolbutton-background-identified"
    }
}
