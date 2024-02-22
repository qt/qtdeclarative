// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ToolSeparator {
    id: control
    objectName: "toolseparator-incomplete"

    contentItem: Item {
        objectName: "toolseparator-contentItem-incomplete"
    }

    background: Item {
        objectName: "toolseparator-background-incomplete"
    }
}
