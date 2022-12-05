// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.SwitchDelegate {
    id: control
    objectName: "switchdelegate-incomplete"

    indicator: Item {
        objectName: "switchdelegate-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "switchdelegate-contentItem-incomplete"
    }

    background: Item {
        objectName: "switchdelegate-background-incomplete"
    }
}
