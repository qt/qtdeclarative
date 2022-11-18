// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.MenuBarItem {
    id: control
    objectName: "menubaritem-incomplete"

    contentItem: Item {
        objectName: "menubaritem-contentItem-incomplete"
    }

    background: Item {
        objectName: "menubaritem-background-incomplete"
    }
}
