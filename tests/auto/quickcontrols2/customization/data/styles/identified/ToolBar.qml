// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ToolBar {
    id: control
    objectName: "toolbar-identified"

    contentItem: Item {
        id: contentItem
        objectName: "toolbar-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "toolbar-background-identified"
    }
}
