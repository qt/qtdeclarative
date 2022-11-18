// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.MenuBar {
    id: control
    objectName: "menubar-incomplete"

    contentItem: Item {
        objectName: "menubar-contentItem-incomplete"
    }

    background: Item {
        objectName: "menubar-background-incomplete"
    }
}
