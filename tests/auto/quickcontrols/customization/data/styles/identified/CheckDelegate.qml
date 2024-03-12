// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.CheckDelegate {
    id: control
    objectName: "checkdelegate-identified"

    indicator: Item {
        id: indicator
        objectName: "checkdelegate-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "checkdelegate-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "checkdelegate-background-identified"
    }
}
