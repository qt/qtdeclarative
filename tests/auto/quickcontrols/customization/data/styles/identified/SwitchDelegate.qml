// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.SwitchDelegate {
    id: control
    objectName: "switchdelegate-identified"

    indicator: Item {
        id: indicator
        objectName: "switchdelegate-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "switchdelegate-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "switchdelegate-background-identified"
    }
}
