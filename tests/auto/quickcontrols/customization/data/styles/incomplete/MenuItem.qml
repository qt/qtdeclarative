// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.MenuItem {
    id: control
    objectName: "menuitem-incomplete"

    arrow: Item {
        objectName: "menuitem-arrow-incomplete"
    }

    indicator: Item {
        objectName: "menuitem-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "menuitem-contentItem-incomplete"
    }

    background: Item {
        objectName: "menuitem-background-incomplete"
    }
}
