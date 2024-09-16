// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Menu {
    id: control
    objectName: "menu-override"

    contentItem: Item {
        objectName: "menu-contentItem-override"
    }

    background: Item {
        objectName: "menu-background-override"
    }
}
