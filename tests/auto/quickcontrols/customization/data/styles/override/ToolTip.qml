// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.ToolTip {
    id: control
    objectName: "tooltip-override"

    contentItem: Item {
        objectName: "tooltip-contentItem-override"
    }

    background: Item {
        objectName: "tooltip-background-override"
    }
}
