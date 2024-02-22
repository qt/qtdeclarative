// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.RadioDelegate {
    id: control
    objectName: "radiodelegate-override"

    indicator: Item {
        objectName: "radiodelegate-indicator-override"
    }

    contentItem: Item {
        objectName: "radiodelegate-contentItem-override"
    }

    background: Item {
        objectName: "radiodelegate-background-override"
    }
}
