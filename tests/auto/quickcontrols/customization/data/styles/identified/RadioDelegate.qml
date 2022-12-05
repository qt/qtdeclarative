// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RadioDelegate {
    id: control
    objectName: "radiodelegate-identified"

    indicator: Item {
        id: indicator
        objectName: "radiodelegate-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "radiodelegate-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "radiodelegate-background-identified"
    }
}
