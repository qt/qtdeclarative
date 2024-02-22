// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.MenuBarItem {
    id: control
    objectName: "menubaritem-override"

    contentItem: Item {
        objectName: "menubaritem-contentItem-override"
    }

    background: Item {
        objectName: "menubaritem-background-override"
    }
}
