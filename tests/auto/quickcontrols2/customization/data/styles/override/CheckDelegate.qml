// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.CheckDelegate {
    id: control
    objectName: "checkdelegate-override"

    indicator: Item {
        objectName: "checkdelegate-indicator-override"
    }

    contentItem: Item {
        objectName: "checkdelegate-contentItem-override"
    }

    background: Item {
        objectName: "checkdelegate-background-override"
    }
}
