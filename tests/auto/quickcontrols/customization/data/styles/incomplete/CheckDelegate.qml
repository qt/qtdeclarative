// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.CheckDelegate {
    id: control
    objectName: "checkdelegate-incomplete"

    indicator: Item {
        objectName: "checkdelegate-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "checkdelegate-contentItem-incomplete"
    }

    background: Item {
        objectName: "checkdelegate-background-incomplete"
    }
}
