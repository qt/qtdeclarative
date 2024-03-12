// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.ToolSeparator {
    id: control
    objectName: "toolseparator-override"

    contentItem: Item {
        objectName: "toolseparator-contentItem-override"
    }

    background: Item {
        objectName: "toolseparator-background-override"
    }
}
