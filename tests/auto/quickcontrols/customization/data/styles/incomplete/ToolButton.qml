// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
