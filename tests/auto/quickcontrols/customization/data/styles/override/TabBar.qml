// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.TabBar {
    id: control
    objectName: "tabbar-override"

    contentItem: Item {
        objectName: "tabbar-contentItem-override"
    }

    background: Item {
        objectName: "tabbar-background-override"
    }
}
