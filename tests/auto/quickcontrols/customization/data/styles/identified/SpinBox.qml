// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.SpinBox {
    id: control
    objectName: "spinbox-identified"

    up.indicator: Item {
        id: upIndicator
        objectName: "spinbox-up.indicator-identified"
    }

    down.indicator: Item {
        id: downIndicator
        objectName: "spinbox-down.indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "spinbox-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "spinbox-background-identified"
    }
}
