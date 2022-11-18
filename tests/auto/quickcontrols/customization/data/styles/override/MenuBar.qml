// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.MenuBar {
    id: control
    objectName: "menubar-override"

    contentItem: Item {
        objectName: "menubar-contentItem-override"
    }

    background: Item {
        objectName: "menubar-background-override"
    }
}
