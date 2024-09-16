// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.MenuBar {
    id: control
    objectName: "menubar-identified"

    contentItem: Item {
        id: contentItem
        objectName: "menubar-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "menubar-background-identified"
    }
}
