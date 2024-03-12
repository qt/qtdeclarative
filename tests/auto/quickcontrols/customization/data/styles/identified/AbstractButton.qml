// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.AbstractButton {
    id: control
    objectName: "abstractbutton-identified"

    indicator: Item {
        id: indicator
        objectName: "abstractbutton-indicator-identified"
        Accessible.name: objectName
    }

    contentItem: Item {
        id: contentItem
        objectName: "abstractbutton-contentItem-identified"
        Accessible.name: objectName
    }

    background: Item {
        id: background
        objectName: "abstractbutton-background-identified"
        Accessible.name: objectName
    }
}
