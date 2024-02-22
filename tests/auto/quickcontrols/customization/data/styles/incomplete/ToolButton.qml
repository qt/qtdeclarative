// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ToolButton {
    id: control
    objectName: "toolbutton-incomplete"

    contentItem: Item {
        objectName: "toolbutton-contentItem-incomplete"
    }

    background: Item {
        objectName: "toolbutton-background-incomplete"
    }
}
