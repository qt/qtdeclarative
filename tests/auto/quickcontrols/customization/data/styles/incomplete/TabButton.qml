// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.TabButton {
    id: control
    objectName: "tabbutton-incomplete"

    contentItem: Item {
        objectName: "tabbutton-contentItem-incomplete"
    }

    background: Item {
        objectName: "tabbutton-background-incomplete"
    }
}
