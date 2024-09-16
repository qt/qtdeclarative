// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Drawer {
    id: control
    objectName: "drawer-override"

    contentItem: Item {
        objectName: "drawer-contentItem-override"
    }

    background: Item {
        objectName: "drawer-background-override"
    }
}
