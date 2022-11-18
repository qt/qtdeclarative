// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.TabBar {
    id: control
    objectName: "tabbar-incomplete"

    contentItem: Item {
        objectName: "tabbar-contentItem-incomplete"
    }

    background: Item {
        objectName: "tabbar-background-incomplete"
    }
}
