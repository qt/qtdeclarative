// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ToolTip {
    id: control
    objectName: "tooltip-incomplete"

    contentItem: Item {
        objectName: "tooltip-contentItem-incomplete"
    }

    background: Item {
        objectName: "tooltip-background-incomplete"
    }
}
