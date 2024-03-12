// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.SwitchDelegate {
    id: control
    objectName: "switchdelegate-override"

    indicator: Item {
        objectName: "switchdelegate-indicator-override"
    }

    contentItem: Item {
        objectName: "switchdelegate-contentItem-override"
    }

    background: Item {
        objectName: "switchdelegate-background-override"
    }
}
