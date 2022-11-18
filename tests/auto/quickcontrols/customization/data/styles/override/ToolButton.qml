// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.ToolButton {
    id: control
    objectName: "toolbutton-override"

    contentItem: Item {
        objectName: "toolbutton-contentItem-override"
    }

    background: Item {
        objectName: "toolbutton-background-override"
    }
}
