// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.MenuItem {
    id: control
    objectName: "menuitem-identified"

    arrow: Item {
        id: arrow
        objectName: "menuitem-arrow-identified"
    }

    indicator: Item {
        id: indicator
        objectName: "menuitem-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "menuitem-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "menuitem-background-identified"
    }
}
