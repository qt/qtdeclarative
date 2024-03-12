// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Menu {
    id: control
    objectName: "menu-incomplete"

    contentItem: Item {
        objectName: "menu-contentItem-incomplete"
    }

    background: Item {
        objectName: "menu-background-incomplete"
    }
}
