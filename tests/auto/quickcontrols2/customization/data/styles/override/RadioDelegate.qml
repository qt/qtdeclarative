// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
