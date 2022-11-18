// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.MenuBarItem {
    id: control
    objectName: "menubaritem-identified"

    contentItem: Item {
        id: contentItem
        objectName: "menubaritem-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "menubaritem-background-identified"
    }
}
