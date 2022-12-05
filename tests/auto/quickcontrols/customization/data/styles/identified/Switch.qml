// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control
    objectName: "switch-identified"

    indicator: Item {
        id: indicator
        objectName: "switch-indicator-identified"
    }

    contentItem: Item {
        id: contentItem
        objectName: "switch-contentItem-identified"
    }
}
