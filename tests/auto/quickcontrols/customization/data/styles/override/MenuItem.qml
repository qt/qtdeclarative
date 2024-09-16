// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.MenuItem {
    id: control
    objectName: "menuitem-override"

    arrow: Item {
        objectName: "menuitem-arrow-override"
    }

    indicator: Item {
        objectName: "menuitem-indicator-override"
    }

    contentItem: Item {
        objectName: "menuitem-contentItem-override"
    }

    background: Item {
        objectName: "menuitem-background-override"
    }
}
