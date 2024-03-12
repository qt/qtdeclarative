// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Menu {
    id: control
    objectName: "menu-identified"

    contentItem: Item {
        id: contentItem
        objectName: "menu-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "menu-background-identified"
    }
}
